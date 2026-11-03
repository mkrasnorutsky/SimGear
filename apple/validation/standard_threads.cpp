// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OneClickPilot contributors
#include <simgear/threads/SGThread.hxx>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <future>
#include <stdexcept>
#include <vector>
using namespace std::chrono_literals;

class Basic final : public SGThread {
public:
    ~Basic() override { join(); }
    std::atomic<int> count{0};
    std::atomic<bool> selfJoinRejected{false};
    void run() override {
        assert(current() == current());
        try { join(); } catch (const std::logic_error&) { selfJoinRejected = true; }
        ++count;
    }
};
class Exclusive final : public SGExclusiveThread {
public:
    ~Exclusive() override { terminate(); }
    std::promise<void> entered;
    std::shared_future<void> unblock;
    std::atomic<int> calls{0};
    bool fail = false;
    int process() override {
        const int n = ++calls;
        if (n == 1) { entered.set_value(); if (unblock.valid()) unblock.wait(); }
        if (fail) throw std::runtime_error("expected");
        return 1;
    }
};

int main()
{
    // A stuck worker fails the test instead of hanging CI indefinitely.
    std::mutex watchdogMutex;
    std::condition_variable watchdogCV;
    bool done = false;
    std::thread watchdog([&] {
        std::unique_lock<std::mutex> lock(watchdogMutex);
        if (!watchdogCV.wait_for(lock, 30s, [&] { return done; })) std::abort();
    });
    Basic basic;
    basic.join();
    for (int i=0; i<100; ++i) {
        assert(basic.start());
        assert(!basic.start()); // Finished-but-unjoined also cannot restart.
        basic.join();
    }
    assert(basic.count == 100 && basic.selfJoinRejected);
    const auto mainID = SGThread::current();
    long otherID = 0;
    std::thread idWorker([&] { otherID = SGThread::current(); });
    idWorker.join();
    assert(mainID > 0 && otherID > 0 && mainID != otherID);

    SGWaitCondition cv;
    SGMutex mutex;
    bool ready = false;
    // Timeout preserves ownership, has no old sticky notifications.
    cv.signal();
    mutex.lock();
    assert(!cv.wait(mutex, 5));
    mutex.unlock();
    std::promise<void> waiting;
    std::thread waiter([&] {
        std::lock_guard<std::mutex> lock(mutex);
        waiting.set_value();
        while (!ready) cv.wait(mutex);
        ready = false;
    });
    waiting.get_future().wait();
    { std::lock_guard<std::mutex> lock(mutex); ready = true; cv.signal(); }
    waiter.join();
    assert(!ready);
    bool broadcastReady = false;
    int arrived = 0, woke = 0;
    std::condition_variable arrivedCV;
    std::vector<std::thread> waiters;
    for (int i=0; i<8; ++i) waiters.emplace_back([&] {
        std::lock_guard<std::mutex> lock(mutex);
        ++arrived; arrivedCV.notify_one();
        while (!broadcastReady) cv.wait(mutex);
        ++woke;
    });
    { std::unique_lock<std::mutex> lock(mutex);
      arrivedCV.wait(lock, [&] { return arrived == 8; });
      broadcastReady = true; cv.broadcast(); }
    for (auto& thread : waiters) thread.join();
    assert(woke == 8);
    // Timed notification must return with the same mutex held.
    ready = false;
    std::thread notifier([&] { std::lock_guard<std::mutex> lock(mutex); ready=true; cv.signal(); });
    { std::lock_guard<std::mutex> lock(mutex); while (!ready) assert(cv.wait(mutex, 1000)); }
    notifier.join();

    // Repeated idle shutdown must wake the worker without invoking process().
    for (int i=0; i<100; ++i) {
        Exclusive idle;
        idle.ensure_running();
        idle.ensure_running();
        assert(idle.is_running());
        idle.terminate();
        idle.terminate();
        assert(!idle.is_running() && idle.calls == 0);
    }
    Exclusive work;
    std::promise<void> finish;
    work.unblock = finish.get_future().share();
    work.ensure_running(); work.release();
    work.entered.get_future().wait();
    for (int i=0; i<100; ++i) work.release(); // Coalesce while busy.
    auto completion = std::async(std::launch::async, [&] { work.awaitCompletion(); });
    auto stopping = std::async(std::launch::async, [&] { work.terminate(); });
    assert(stopping.wait_for(5ms) == std::future_status::timeout);
    assert(completion.wait_for(5ms) == std::future_status::timeout);
    finish.set_value();
    stopping.get(); completion.get();
    assert(work.calls == 1 && !work.is_running());
    work.ensure_running(); work.release(); work.awaitCompletion();
    assert(work.calls == 2);
    work.stop();
    Exclusive error;
    error.fail=true; error.ensure_running(); error.release();
    bool caught=false;
    try { error.awaitCompletion(); } catch (const std::runtime_error&) { caught=true; }
    assert(caught);
    error.terminate();
    // Concurrent release/await against repeated shutdown/restart.
    Exclusive race;
    race.ensure_running();
    std::atomic<bool> quit{false};
    std::thread producer([&] { while (!quit) race.release(); });
    std::thread observer([&] { while (!quit) race.awaitCompletion(); });
    for (int i=0; i<100; ++i) { race.terminate(); race.ensure_running(); }
    quit=true; race.terminate(); producer.join(); observer.join();
    { std::lock_guard<std::mutex> lock(watchdogMutex); done=true; watchdogCV.notify_one(); }
    watchdog.join();
    std::puts("PASS standard threads: lifecycle, mutex ownership, timeout, broadcast, cooperative stop, restart, exception and concurrent stress");
}
