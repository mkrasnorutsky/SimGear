// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OneClickPilot contributors
#include "SGThread.hxx"

#include <atomic>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <system_error>

namespace {
// Detect self-join before acquiring a lock that an owner may hold while joining.
thread_local SGThread* executingThread = nullptr;
}

SGThread::SGThread() = default;
SGThread::~SGThread()
{
    // Joining here is too late: the derived state has already been destroyed.
    // Enforce explicit owner shutdown rather than silently causing use-after-free.
    if (worker.joinable()) std::terminate();
}

bool SGThread::start()
{
    if (executingThread == this) return false;
    std::lock_guard<std::mutex> lock(lifecycle);
    if (worker.joinable()) return false;
    try {
        worker = std::thread([this] { start_routine(this); });
        return true;
    } catch (const std::system_error&) {
        return false;
    }
}

void SGThread::join()
{
    if (executingThread == this)
        throw std::logic_error("SGThread cannot join itself");
    std::lock_guard<std::mutex> lock(lifecycle);
    if (worker.joinable()) worker.join();
}

long SGThread::current()
{
    static std::atomic<unsigned long> next{1};
    thread_local const long identifier = [] {
        const auto value = next.fetch_add(1, std::memory_order_relaxed);
        if (value > static_cast<unsigned long>(std::numeric_limits<long>::max()))
            std::terminate();
        return static_cast<long>(value);
    }();
    return identifier;
}

void* SGThread::start_routine(void* argument)
{
    auto* thread = static_cast<SGThread*>(argument);
    executingThread = thread;
    thread->run(); // Uncaught exceptions have std::thread's terminate semantics.
    executingThread = nullptr;
    return nullptr;
}

SGWaitCondition::SGWaitCondition() = default;
SGWaitCondition::~SGWaitCondition() = default;
void SGWaitCondition::wait(std::mutex& mutex)
{
    std::unique_lock<std::mutex> lock(mutex, std::adopt_lock);
    try { condition.wait(lock); }
    catch (...) { lock.release(); throw; }
    lock.release(); // The caller still owns the mutex.
}

bool SGWaitCondition::wait(std::mutex& mutex, unsigned milliseconds)
{
    std::unique_lock<std::mutex> lock(mutex, std::adopt_lock);
    try {
        const auto result = condition.wait_for(lock, std::chrono::milliseconds(milliseconds));
        lock.release();
        return result != std::cv_status::timeout;
    } catch (...) { lock.release(); throw; }
}
void SGWaitCondition::signal() { condition.notify_one(); }
void SGWaitCondition::broadcast() { condition.notify_all(); }

SGExclusiveThread::SGExclusiveThread() = default;
SGExclusiveThread::~SGExclusiveThread() { terminate(); }

void SGExclusiveThread::ensure_running()
{
    if (executingThread == this)
        throw std::logic_error("SGExclusiveThread cannot restart itself");
    std::lock_guard<std::mutex> owner(control);
    std::lock_guard<std::mutex> lock(state);
    if (started) return;
    stopping = false;
    pending = false;
    failure = nullptr;
    started = start();
}

void SGExclusiveThread::release()
{
    std::lock_guard<std::mutex> lock(state);
    if (!started || stopping || pending || busy) return;
    pending = true;
    workAvailable.notify_one();
}

void SGExclusiveThread::wait()
{
    std::unique_lock<std::mutex> lock(state);
    workAvailable.wait(lock, [this] { return stopping || pending; });
}

void SGExclusiveThread::awaitCompletion()
{
    std::unique_lock<std::mutex> lock(state);
    workComplete.wait(lock, [this] { return !pending && !busy; });
    if (failure) std::rethrow_exception(failure);
}
void SGExclusiveThread::clearAwaitCompletionTime() {}

void SGExclusiveThread::setCompletion()
{
    std::lock_guard<std::mutex> lock(state);
    busy = false;
    workComplete.notify_all();
}

void SGExclusiveThread::run()
{
    std::unique_lock<std::mutex> lock(state);
    for (;;) {
        workAvailable.wait(lock, [this] { return stopping || pending; });
        if (stopping) break;
        pending = false;
        busy = true;
        lock.unlock();
        std::exception_ptr error;
        try { process(); } catch (...) { error = std::current_exception(); }
        lock.lock();
        busy = false;
        if (error) { failure = error; stopping = true; }
        workComplete.notify_all();
    }
    pending = false;
    workComplete.notify_all();
}

void SGExclusiveThread::terminate()
{
    if (executingThread == this)
        throw std::logic_error("SGExclusiveThread cannot stop itself");
    std::lock_guard<std::mutex> owner(control);
    {
        std::lock_guard<std::mutex> lock(state);
        stopping = true;
        pending = false;
        workAvailable.notify_all();
        workComplete.notify_all();
    }
    join(); // No state lock held: process() and completion can finish.
    std::lock_guard<std::mutex> lock(state);
    started = false;
}

bool SGExclusiveThread::stop() { terminate(); return true; }
bool SGExclusiveThread::is_running()
{
    std::lock_guard<std::mutex> lock(state);
    return started && !stopping;
}
