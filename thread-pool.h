#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "Log.h"

namespace util {
    class ThreadPool {
    public:
        static inline std::uint64_t now() noexcept {
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        }
        static void initialize(int count) noexcept;
        static void finalize() noexcept;

    public:
        ThreadPool(int count) noexcept {
            for (int i = 0; i < count; i++) {
                threads_.emplace_back(std::make_unique<std::thread>(&ThreadPool::run_, this));
            }
        }

        ~ThreadPool() noexcept {
            is_terminate_ = true;
            for (auto& thread : threads_) {
                if (thread && thread->joinable()) {
                    thread->join();
                }
            }
            log_trace << "deleting thread pool";
        }

        template <class F, class... Args>
        auto submit_task(std::int64_t timeout, std::function<void()>&& timeout_handler, F&& f, Args&&... args) noexcept -> std::future<decltype(f(args...))> {
            std::int64_t expire_time = (timeout == 0) ? 0 : now() + timeout;
            auto task = std::make_shared<Task>(expire_time);
            if (expire_time) {
                task->timeout_handler = std::move(timeout_handler);
            }
            using return_type = decltype(f(args...));
            auto task_code = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            task->code = [task_code] { (*task_code)(); };
            {
                std::unique_lock<std::mutex> lock{tasks_mutex_};
                tasks_.push_back(task);
                log_info << "thread-pool push_back tasks_:" << tasks_.size();
            }
            wait_condition_.notify_one();
            return task_code->get_future();
        }

        bool wait_for_all_done(int milliseconds = -1) noexcept;

    private:
        struct Task {
            Task(std::uint64_t expireTime) : expire_time(expireTime) {}
            std::uint64_t expire_time{};
            std::function<void()> timeout_handler;
            std::function<void()> code;
        };

        std::shared_ptr<Task> fetch_task_(int milliseconds) noexcept;
        void run_() noexcept;

    private:
        std::atomic<bool> is_terminate_ = false;
        std::vector<std::unique_ptr<std::thread>> threads_;
        std::mutex tasks_mutex_;
        std::deque<std::shared_ptr<Task>> tasks_;
        std::condition_variable wait_condition_;
        std::atomic<int> running_count_ = 0;
    };

    extern std::shared_ptr<ThreadPool> pool;
} // namespace util
