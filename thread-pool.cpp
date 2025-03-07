﻿#include "thread-pool.h"

namespace util {
    bool ThreadPool::wait_for_all_done(int milliseconds) noexcept {
        std::unique_lock<std::mutex> lock{tasks_mutex_};
        auto wait_predicate = [this] { return is_terminate_ || tasks_.empty() && running_count_ == 0; };
        if (milliseconds < 0) {
            wait_condition_.wait(lock, wait_predicate);
        } else {
            wait_condition_.wait_for(lock, std::chrono::milliseconds(milliseconds), wait_predicate);
        }
        return tasks_.empty();
    }

    std::shared_ptr<ThreadPool::Task> ThreadPool::fetch_task_(int milliseconds) noexcept {
        std::shared_ptr<ThreadPool::Task> result = nullptr;
        if (std::unique_lock<std::mutex> lock{tasks_mutex_}; wait_condition_.wait_for(lock, std::chrono::milliseconds(milliseconds), [this] { return !tasks_.empty(); })) {
            if (!tasks_.empty()) {
                result = std::move(tasks_.front());
                tasks_.pop_front();
            }
        } else {
            log_trace << "thread-pool get task timeout after " << milliseconds << " milliseconds";
        }
        return result;
    }

    void ThreadPool::run_() noexcept {
        while (!is_terminate_) {
            auto task = fetch_task_(500);
            if (task) {
                ++running_count_;
                try {
                    if (task->expire_time != 0 && task->expire_time < now()) {
                        log_info << "thread-pool run task->timeout_handler()";
                        if (task->timeout_handler) {
                            task->timeout_handler();
                        }
                    } else {
                        log_trace << "thread-pool run task begin";
                        task->code();
                        log_trace << "thread-pool run task finish";
                    }
                } catch (...) {}
                --running_count_;
            }
            if (std::unique_lock<std::mutex> lock{tasks_mutex_}; tasks_.empty() && running_count_ == 0) {
                wait_condition_.notify_all();
            }
        }
    }
} // namespace util
