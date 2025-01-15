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

#include "object-pool.h"

namespace util {
    namespace {
        std::uint64_t now() noexcept {
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        }

        struct Task {
            Task(std::uint64_t expireTime) : expire_time(expireTime) {}
            std::uint64_t expire_time{};
            std::function<void()> timeout_handler;
            std::function<void()> code;
        };

        std::mutex tasks_mutex;
        std::deque<std::shared_ptr<Task>> tasks;
    } // namespace

    std::pair<bool, std::shared_ptr<std::thread>> create_thread() noexcept;

    class ThreadPool : public ObjectPool<std::shared_ptr<std::thread>> {
    public:
        ThreadPool(int count) noexcept
            : ObjectPool(count, count, create_thread, [](std::shared_ptr<std::thread> thread) {
                  if (thread && thread->joinable()) {
                      thread->join();
                  }
              }) {}
        ~ThreadPool() noexcept override {
            log_trace << "deleting thread pool";
        }
    };

    namespace ThreadPoolManager {
        void initialize(int count) noexcept;
        void finalize() noexcept;
        template <class F, class... Args>
        auto submit_task(std::int64_t timeout, std::function<void()>&& timeout_handler, F&& f, Args&&... args) noexcept -> std::future<decltype(f(args...))> {
            using return_type = decltype(f(args...));
            auto task_code = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            std::int64_t expire_time = (timeout == 0) ? 0 : now() + timeout;
            auto task = std::make_shared<Task>(expire_time);
            task->code = [task_code]() { (*task_code)(); };
            std::unique_lock<std::mutex> lock{tasks_mutex};
            tasks.push_back(task);
            log_info << "thread-pool push_back tasks_:" << tasks.size();
            // wait_condition.notify_one();
            return task_code->get_future();
        }
    } // namespace ThreadPoolManager

    namespace {
        void print_siginfo(siginfo_t* si) {}

        void handler(int sig, siginfo_t* si, void* uc) {
            printf("Caught signal %d\n", sig);
            print_siginfo(si);
            signal(sig, SIG_IGN);
        }
    } // namespace

    inline void timerProc(int signo) {}

    class thread_pool {
    public:
        thread_pool() noexcept {}

        ~thread_pool() noexcept {}

        bool init(std::size_t number) noexcept {
            bool result = false;
            if (threads_.empty()) {
                thread_number_ = number;
            }
            return result;
        }

        bool start() noexcept {
            bool result = false;
            if (threads_.empty()) {
                for (std::size_t i = 0; i < thread_number_; ++i) {
                    threads_.push_back(new std::thread([this] { run(); }));
                }
                result = true;
            }
            return result;
        }

        template <class F, class... Args> auto exec(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
            return exec(0, []() {}, f, args...);
        }

        template <class F, class... Args> auto exec(std::uint64_t timeout, std::function<void()>&& timeout_handler, F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
            using return_type = decltype(f(args...));
            auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            std::uint64_t expire_time = (timeout == 0) ? 0 : now() + timeout;
            auto task_wrap = std::make_shared<Task>(expire_time);
            task_wrap->code = [task]() { (*task)(); };
            // log_debug << "before exec";
            {
                std::unique_lock<std::mutex> lock{mutex_};
                tasks_.push_back(task_wrap);
                log_info << "thread-pool push_back tasks_:" << tasks_.size();
                wait_condition_.notify_one();
            }
            // log_debug << "after exec";
            return task->get_future();
        }

        bool wait_for_all_done(int milliseconds = -1) noexcept {
            bool result = false;
            log_info << "wait_for_all_done thread-pool  wait_for_all_done start\n";
            if (std::unique_lock<std::mutex> lock{mutex_}; tasks_.empty()) {
                log_info << "thread-pool empty for return\n";
                result = true;
            }
            if (std::unique_lock<std::mutex> lock{mutex_}; milliseconds < 0) {
                log_info << "wait forever condition return\n";
                wait_condition_.wait(lock, [this] { return is_terminate_ || tasks_.empty(); });
                result = true;
            } else {
                log_info << "wait " << milliseconds << " condition return\n";
                result = wait_condition_.wait_for(lock, std::chrono::milliseconds(milliseconds), [this] { return is_terminate_ || tasks_.empty(); });
            }

            log_info << "wait_for_all_done finish ";
            return result;
        }

        void stop() noexcept {
            {
                std::unique_lock<std::mutex> lock{mutex_};
                is_terminate_ = true;
                wait_condition_.notify_all();
            }
            std::size_t thread_count = threads_.size();
            for (std::size_t i = 0; i < thread_count; ++i) {
                if (threads_[i]->joinable()) {
                    threads_[i]->join();
                }
                delete threads_[i];
                threads_[i] = nullptr;
            }
            threads_.clear();
        }

        std::size_t get_thread_number() noexcept {
            return threads_.size();
        }

        std::size_t get_task_number() noexcept {
            // std::unique_lock<std::mutex> lock{mutex_};
            return tasks_.size();
        }

    private:
        struct Task {
            Task(std::uint64_t expireTime) : expire_time(expireTime) {}
            std::uint64_t expire_time{};
            std::function<void()> timeout_handler;
            std::function<void()> code;
        };

        bool get(std::shared_ptr<Task>& task) noexcept {
            bool result = false;

            std::unique_lock<std::mutex> lock{mutex_};
            wait_condition_.wait(lock, [this] { return is_terminate_ || !tasks_.empty(); });
            if (!is_terminate_) {
                if (!tasks_.empty()) {
                    task = std::move(tasks_.front());
                    tasks_.pop_front();
                    // log_trace << "thread-pool get after pop_front tasks size: " << tasks_.size();
                    result = true;
                }
            }
            return result;
        }

        bool is_terminate() noexcept {
            return is_terminate_;
        }

        void run() noexcept {
            // log_trace << "is_terminate:" << is_terminate();
            while (!is_terminate()) {
                // log_trace << "run";
                std::shared_ptr<Task> task = nullptr;
                bool ok = get(task);
                // log_trace << "get(task)";
                if (ok && task) {
                    ++running_count_;
                    try {
                        if (task->expire_time != 0 && task->expire_time < now()) {
                            log_info << "thread-pool run task->timeout_handler()";
                            if (task->timeout_handler) {
                                task->timeout_handler();
                            }
                        } else {
                            log_trace << "thread-pool run task begin";
                            if (task->code) {
                                task->code();
                            }
                            log_trace << "thread-pool run task finish";
                        }
                    } catch (const std::exception& e) {
                        log_error << "thread-pool run :" << e.what();
                    }
                    --running_count_;
                    std::unique_lock<std::mutex> lock{mutex_};
                    if (running_count_ == 0 && tasks_.empty()) {
                        log_trace << "wait_condition_.notify_all()";
                        wait_condition_.notify_all();
                    }
                } else {
                    // log_error << " get task error";
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
        }

        std::vector<std::thread*> threads_;
        std::size_t thread_number_{};
        std::deque<std::shared_ptr<Task>> tasks_;
        std::mutex mutex_;
        bool is_terminate_ = false;
        std::condition_variable wait_condition_;
        std::atomic<int> running_count_ = 0;
    };
} // namespace util
