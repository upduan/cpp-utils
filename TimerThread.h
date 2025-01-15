#pragma once

namespace util::TimerThread {
    void init() noexcept;

    void set(HANDLE event, int timeout) noexcept;

    void clear(HANDLE event) noexcept;

    void uninit() noexcept;
} // namespace util::TimerThread
