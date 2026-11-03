// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OneClickPilot contributors
// Independent implementation of the SGThread compatibility API.
#pragma once

#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>

using SGMutex = std::mutex;

// Owner-side start/join are serialized. A derived destructor must stop and join
// before destroying anything accessed by run(). Destruction never detaches.
class SGThread {
public:
    SGThread();
    bool start();
    void join();
    static long current(); // Process-local identifier, not an OS thread handle.
    SGThread(const SGThread&) = delete;
    SGThread& operator=(const SGThread&) = delete;
protected:
    virtual ~SGThread();
    virtual void run() = 0;
    static void* start_routine(void*);
private:
    std::mutex lifecycle;
    std::thread worker;
};

// Standard condition-variable semantics: caller owns the supplied mutex,
// returns owning it, and must test its predicate in a loop. Notifications are
// not queued. Timed wait returns false on timeout; spurious wakes are allowed.
class SGWaitCondition final {
public:
    SGWaitCondition();
    ~SGWaitCondition();
    void wait(std::mutex&);
    bool wait(std::mutex&, unsigned milliseconds);
    void signal();
    void broadcast();
    SGWaitCondition(const SGWaitCondition&) = delete;
    SGWaitCondition& operator=(const SGWaitCondition&) = delete;
private:
    std::condition_variable condition;
};

// At most one outstanding process() invocation. Duplicate releases coalesce.
// terminate()/stop() discard queued work, wake idle waits and join in-flight
// work. They cannot interrupt process(); it must eventually return. The derived
// destructor must terminate before destroying its own state. Owner-side
// ensure_running/terminate must not be called from process().
class SGExclusiveThread : public SGThread {
public:
    SGExclusiveThread();
    ~SGExclusiveThread() override;
    void release();
    void wait();
    void clearAwaitCompletionTime(); // Retained compatibility hook.
    virtual void awaitCompletion();
    void setCompletion();
    virtual int process() = 0;
    void run() override;
    void terminate();
    bool stop();
    void ensure_running();
    bool is_running();
private:
    std::mutex control;
    std::mutex state;
    std::condition_variable workAvailable;
    std::condition_variable workComplete;
    bool started = false;
    bool stopping = false;
    bool pending = false;
    bool busy = false;
    std::exception_ptr failure;
};
