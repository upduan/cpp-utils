#include "thread-pool.h"

namespace util {
    namespace {
        std::atomic<bool> is_quit = false;

        std::shared_ptr<Task> fetch_task_impl() noexcept {
            std::shared_ptr<Task> result = nullptr;
            if (std::unique_lock<std::mutex> lock{tasks_mutex}; !tasks.empty()) {
                result = std::move(tasks.front());
                tasks.pop_front();
            }
            return result;
        }

        std::shared_ptr<Task> fetch_task() noexcept {
            std::shared_ptr<Task> result = nullptr;
            while (!is_quit) {
                result = fetch_task_impl();
                if (result) {
                    std::unique_lock<std::mutex> lock{tasks_mutex};
                    log_trace << "thread-pool get after pop_front tasks size: " << tasks.size();
                    break;
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(30));
                }
            }
            return result;
        }
    } // namespace

    std::pair<bool, std::shared_ptr<std::thread>> create_thread() noexcept {
        auto thread = std::make_shared<std::thread>([] {
            while (!is_quit) {
                auto task = fetch_task();
                try {
                    if (task) {
                        if (task->expire_time != 0 && task->expire_time < now()) {
                            log_info << "thread-pool run task->timeout_handler()";
                            task->timeout_handler();
                        } else {
                            log_trace << "thread-pool run task begin";
                            task->code();
                            log_trace << "thread-pool run task finish";
                        }
                    }
                } catch (...) {}
            }
        });
        return std::pair<bool, std::shared_ptr<std::thread>>{true, thread};
    }

    namespace ThreadPoolManager {
        namespace {
            std::shared_ptr<ThreadPool> pool = nullptr;
        }

        void initialize(int count) noexcept {
            pool = std::make_shared<ThreadPool>(count);
        }

        void finalize() noexcept {
            pool = nullptr;
        }
    } // namespace ThreadPoolManager
} // namespace util

void thread_pool_test() {
    util::ThreadPoolManager::initialize(10); // call only once
    for (int i = 0; i < 10; ++i) {
        // call on need
        util::ThreadPoolManager::submit_task(
            0 /*timeout*/,
            [] {
                // timeout process
            },
            [] {
                // task process
            });
    }
    util::ThreadPoolManager::finalize(); // call only once
}
