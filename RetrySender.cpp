#include "RetrySender.h"

#include <chrono> // for std::chrono_literals
#include <map> // for std::map<>
#include <thread> // for std::this_thread::sleep_for

namespace util {
    namespace {
        /*
        std::map<UINT_PTR, RetrySender*> timers;

        void __stdcall proc(HWND window, UINT wmTimerMsg, UINT_PTR timerId, DWORD currentSystemTime) {
            RetrySender* self = nullptr;
            if (timers.find(timerId) != timers.end()) {
                self = timers[timerId];
            }
            if (self) {
                if (self->timeouts().empty()) {
                    self->setFailure();
                    if (self->failure()) {
                        self->failure()();
                    }
                    timers.erase(self->timerId());
                } else {
                    self->timeouts().pop_front();
                    self->sender()(self->message());
                    if (!self->timeouts().empty()) {
                        timers.erase(self->timerId());
                        self->timerId(::SetTimer(nullptr, self->timerId(), self->timeouts()[0], proc));
                        timers[self->timerId()] = self;
                    }
                }
            }
        }
        */
    } // namespace

    RetrySender::RetrySender(std::function<void(WebSocket::Message&& message)>&& sender, WebSocket::Message&& message, std::deque<int>&& timeouts,
        std::function<void()>&& failure) noexcept
        : sender_(std::move(sender)), message_(std::move(message)), timeouts_(std::move(timeouts)), failure_(std::move(failure)) {
        /*
        if (!timeouts.empty()) {
            timerId_ = ::SetTimer(nullptr, 0, timeouts_[0], proc);
            timers[timerId_] = this;
        }
        sender_({message_.type, message_.content});
        */
    }

    void RetrySender::setSuccess() noexcept {
        status_ = Status::SUCCESS;
        /*
        ::KillTimer(nullptr, timerId_);
        timers.erase(timerId_);
        */
    }

    RetrySender::~RetrySender() noexcept {
        while (true) {
            if (status_ == Status::PROGRESS) {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(20ms);
            } else {
                // timers.erase(timerId_);
                break;
            }
        }
    }
} // namespace util
