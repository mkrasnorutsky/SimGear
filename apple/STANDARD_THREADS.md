# Standard C++ thread compatibility layer

`simgear/threads/SGThread.hxx` and `.cxx` are an independent MIT implementation of the public SGThread, SGWaitCondition and SGExclusiveThread interfaces. See `simgear/threads/LICENSE.MIT`. This does not change licenses of other SimGear files, SGQueue, SGGuard, or their users. There is no dependency on SimGear logging, timing, native pthread cancellation, or Emesary.

## Lifetime and cancellation

- `start()` returns true on success; a started thread must be joined before reuse. `join()` on an unstarted/joined object does nothing. Self-join throws `std::logic_error`. Owner-side lifecycle calls are serialized.
- No forced cancellation or detach exists. The owner sets its application stop predicate under its mutex (or uses an atomic), wakes any wait and joins. Blocking external I/O still needs its own timeout/cancellation. A stop request cannot interrupt it.
- Every derived destructor must stop/join before destroying worker-visible members. A base SGThread destroyed with a joinable worker terminates the process instead of silently allowing use-after-free. Joining in the base destructor would already be too late to protect derived members.
- SGExclusiveThread admits one work item, coalesces duplicate releases while pending/busy, and discards unstarted work on stop. Stop wakes an idle worker without calling process(), then joins any in-flight process(). `stop()` actually stops; `terminate()` is idempotent; `ensure_running()` permits restart after joining. Lifecycle methods must be called by the owner, never from process().
- Exclusive-worker process exceptions complete/wake waiters and stop further dispatch. `awaitCompletion()` rethrows the captured exception; shutdown joins without rethrowing. Restart clears the saved failure.
- `current()` is a stable process-local positive ID, not a native handle.

## Condition variables

`wait()` atomically releases the caller's locked mutex and reacquires it before returning. Notifications are not stored tokens and spurious wakeups are permitted: callers must test a predicate in a loop while holding that same mutex. Timed waits return false on timeout. `broadcast()` wakes all current waiters. These intentionally replace the old internal sticky single-boolean signaling and always-true timed return.

SGQueue and BVHPager already use predicate loops. The asynchronous socket resolver releases its state mutex during lookup, so it now rechecks its shutdown and pending-lookup predicates before waiting. OneClickPilot's navigation rebuild/search cancellation flags are atomic in the application project. Cancellation remains cooperative; no bounded shutdown is promised for a blocking lookup or process().

`wait()`, `setCompletion()`, `run()` and the no-op `clearAwaitCompletionTime()` remain compatibility hooks; normal exclusive-worker callers use ensure_running/release/awaitCompletion/terminate. Do not externally call setCompletion while process is still accessing a work buffer.

## ABI and validation

The class layouts changed. All SimGear sources and application clients must be rebuilt against the matching headers; old archives cannot be mixed with these headers. The XCFramework includes arm64 iOS and arm64/x86_64 Simulator slices and the MIT notice.

Run `python3 apple/scripts/validate_standard_threads.py` for ASan/UBSan and TSan host tests, or pass `--artifact /path/SimGear.xcframework --run-simulator` to additionally link all three architectures and run against the arm64 Simulator archive. The regular XCFramework validator includes these checks. Tests cover restart, self-join, mutex ownership, timeout, broadcast, idle stop, in-flight stop, duplicate release, completion, exceptions and concurrent lifecycle stress.
