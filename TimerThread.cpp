#include "TimerThread.h"

#include <atomic>
#include <deque>
#include <mutex>
#include <thread>

#include "Log.h"

namespace util::TimerThread {
    namespace {
        struct Item {
            int originTimeout;
            HANDLE event;
            long long timeout;
            long long absoluteTimeout;
        };
        std::deque<Item> observers;
        // std::vector<Item> observers;
        std::mutex observersMutex;
        std::atomic<bool> isQuit = false;
        std::atomic<bool> isFinal = false;
        std::atomic<bool> isEnd = false;
        std::thread* timerThread = nullptr;
        LARGE_INTEGER freq;
        long long counterPreMillisecond = 0;
        LARGE_INTEGER previousTime;
        LARGE_INTEGER currentTime;

        void setInternal(Item&& item) noexcept {
            std::lock_guard<std::mutex> lock{observersMutex};
            auto it = std::find_if(observers.begin(), observers.end(), [&item](auto it) { return it.absoluteTimeout > item.absoluteTimeout; });
            observers.insert(it, std::move(item));
        }
    } // namespace

    void init() noexcept {
        QueryPerformanceFrequency(&freq);
        counterPreMillisecond = (long long)(freq.QuadPart / 1000.0);
        if (!timerThread) {
            isQuit = false;
            isFinal = false;
            timerThread = new std::thread([]() {
                while (!isQuit) {
                    Item item;
                    long long difference = 0;
                    if (std::lock_guard<std::mutex> lock{observersMutex}; observers.empty()) {
                        continue;
                    } else {
                        item = observers[0];
                        difference = item.absoluteTimeout - previousTime.QuadPart;
                        // util::Log::log("%d timer, timeout is %lld, to is %lld, difference is %lld", id, timeout, o.to, difference);
                    }
                    while (true) {
                        QueryPerformanceCounter(&currentTime);
                        if ((currentTime.QuadPart - previousTime.QuadPart) >= difference) {
                            // util::Log::log("%d time reach. setevent", id);
                            previousTime = currentTime;
                            if (isEnd) {
                                isEnd = false;
                            } else {
                                SetEvent(item.event);
                                // QueryPerformanceCounter(&currentTime);
                                item.absoluteTimeout = currentTime.QuadPart + item.timeout;
                                setInternal(std::move(item));
                            }
                            {
                                std::lock_guard<std::mutex> lock{observersMutex};
                                observers.pop_front();
                                // observers.erase(observers.begin());
                            }
                            break;
                        } else {
                            // util::Log::log("not reach time. wait");
                        }
                    }
                }
                isFinal = true;
            });
            timerThread->detach();
        }
    }

    void set(HANDLE event, int originTimeout) noexcept {
        long long timeout = (long long)((originTimeout / 1000.0) * freq.QuadPart);
        QueryPerformanceCounter(&currentTime);
        long long absoluteTimeout = currentTime.QuadPart + timeout;
        if (std::lock_guard<std::mutex> lock{observersMutex}; observers.empty()) {
            QueryPerformanceCounter(&previousTime);
        } else {
            if (absoluteTimeout <= observers[0].absoluteTimeout) {
                absoluteTimeout = observers[0].absoluteTimeout + counterPreMillisecond;
            }
        }
        setInternal(Item{originTimeout, event, timeout, absoluteTimeout});
        // util::Log::log("set: timers count is %d", observers.size());
    }

    void clear(HANDLE event) noexcept {
        std::lock_guard<std::mutex> lock{observersMutex};
        if (auto it = std::find_if(observers.begin(), observers.end(), [event](auto item) { return item.event == event; }); it != observers.end()) {
            if (it == observers.begin()) {
                isEnd = true;
            } else {
                observers.erase(it);
            }
        }
        // util::Log::log("clear: timers count is %d", observers.size());
    }

    void uninit() noexcept {
        if (std::lock_guard<std::mutex> lock{observersMutex}; !observers.empty()) {
            observers.clear();
        }
        isQuit = true;
        while (!isFinal) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        delete timerThread;
        timerThread = nullptr;
    }
} // namespace util::TimerThread
