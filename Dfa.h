#pragma once

#include <any> // for std::any
#include <deque> // for std::deque<>
#include <functional> // for std::function<>
#include <map> // for std::map<>
#include <memory> // for std::shared_ptr<>, std::make_shared<>
#include <mutex> // for std::mutex
#include <shared_mutex>
#include <string> // for std::string
#include <thread> // for std::thread

#include "Log.h"

namespace util {
    template <typename TState, typename TEvent> class Dfa {
    public:
        Dfa(TState state) noexcept : current_(state) {
            running_ = std::make_shared<std::thread>([](Dfa* self) { self->run_(); }, this);
        }

        ~Dfa() noexcept {
            stop();
        }

        void stop() noexcept {
            log_info << "dfa " << label_ << " start finally";
            isQuit_ = true;
            if (running_ && running_->joinable()) {
                running_->join();
            }
            running_ = nullptr;
            log_info << "dfa " << label_ << " finally end";
        }

        void setLabel(std::string const& label) noexcept {
            label_ = label;
        }

        void setRules(std::map<TState, std::map<TEvent, std::pair<TState, std::function<void(std::any args)>>>> rules) noexcept {
            rules_ = rules;
        }

        void registerRule(TState start, TEvent event, TState stop, std::function<void(std::any args)>&& action) noexcept {
            std::pair<TState, std::function<void(std::any args)>> item{stop, action};
            if (rules_.find(start) == rules_.end()) {
                std::map<TEvent, std::pair<TState, std::function<void(std::any args)>>> jumpTable;
                jumpTable.emplace(event, item);
                rules_.emplace(start, jumpTable);
            } else {
                auto& jumpTable = rules_[start];
                if (jumpTable.find(event) != jumpTable.end()) {
                    jumpTable.erase(event);
                }
                jumpTable.emplace(event, item);
            }
        }

        void fire(TEvent event, std::any args = nullptr) noexcept {
            std::lock_guard<std::mutex> guard(mutex_);
            eventQueue_.push_back({event, args});
        }

        TState getCurrent() const noexcept {
            std::shared_lock<std::shared_mutex> guard(state_mutex_);
            return current_;
        }

        TEvent getEvent() const noexcept {
            return event_;
        }

    private:
        void run_() noexcept {
           // static void* self = this;
            while (!isQuit_) {
                bool hasEvent = false;
                std::pair<TEvent, std::any> e;
                if (std::lock_guard<std::mutex> guard(mutex_); !eventQueue_.empty()) {
                    hasEvent = true;
                    e = eventQueue_.front();
                    eventQueue_.pop_front();
                }
                if (hasEvent) {
                    auto state = (TState)current_;
                    event_ = e.first;
                    std::string current_state = to_string(current_);
                    std::string event = to_string(e.first);
                    std::string next_state = current_state;
                    auto it = rules_.find(state);
                    if (it != rules_.end()) {
                        auto& jumpTable = it->second;
                        auto jIt = jumpTable.find(e.first);
                        if (jIt != jumpTable.end()) {
                            auto& r = jIt->second;
                            next_state = to_string(r.first);
                            {
                                std::unique_lock<std::shared_mutex> guard(state_mutex_);
                                current_ = r.first;
                            }
                            if (r.second) {
                                 if (event != "ReportPose") {
                                    log_debug << label_ << " find&run current_state: " << current_state << ", event: " << event << ", next_state: " << next_state;
                                 }
                                r.second(e.second);
                            }
                        }
                    } else if (event != "ReportPose") {
                        log_debug << label_ << " not find rulers current_state: " << current_state << ", event: " << event << ", next_state: " << next_state;
                    }
                }
                // using namespace std::chrono_literals;
                // std::this_thread::sleep_for(1ms);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

    private:
        std::shared_ptr<std::thread> running_ = nullptr;
        mutable std::shared_mutex state_mutex_;
        volatile TState current_;
        volatile TEvent event_;
        std::map<TState, std::map<TEvent, std::pair<TState, std::function<void(std::any args)>>>> rules_;
        std::mutex mutex_;
        std::string label_ = "dfa";
        std::deque<std::pair<TEvent, std::any>> eventQueue_;
        volatile bool isQuit_ = false;
    };
} // namespace util
