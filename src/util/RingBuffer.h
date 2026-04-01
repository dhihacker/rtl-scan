#pragma once
#include <array>
#include <condition_variable>
#include <mutex>
#include <optional>

template <typename T, std::size_t N>
class RingBuffer {
public:
    void push(T item) {
        std::unique_lock lock(m_mutex);
        m_data[m_head] = std::move(item);
        m_head = (m_head + 1) % N;
        if (m_size < N)
            ++m_size;
        lock.unlock();
        m_cv.notify_one();
    }

    // Non-blocking pop — returns nullopt if empty.
    std::optional<T> try_pop() {
        std::lock_guard lock(m_mutex);
        if (m_size == 0)
            return std::nullopt;
        std::size_t tail = (m_head + N - m_size) % N;
        --m_size;
        return std::move(m_data[tail]);
    }

    // Blocking pop with timeout.
    std::optional<T> wait_pop(std::chrono::milliseconds timeout) {
        std::unique_lock lock(m_mutex);
        if (!m_cv.wait_for(lock, timeout, [this] { return m_size > 0; }))
            return std::nullopt;
        std::size_t tail = (m_head + N - m_size) % N;
        --m_size;
        return std::move(m_data[tail]);
    }

    std::size_t size() const {
        std::lock_guard lock(m_mutex);
        return m_size;
    }

private:
    std::array<T, N>        m_data{};
    std::size_t             m_head{0};
    std::size_t             m_size{0};
    mutable std::mutex      m_mutex;
    std::condition_variable m_cv;
};
