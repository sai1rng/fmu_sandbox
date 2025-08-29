/**
 * @file ThreadSafeQueue.hpp
 * @brief A generic, thread-safe queue for inter-thread communication.
 */
#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template <typename T>
class ThreadSafeQueue {
public:
    // Pushes a new value onto the queue and notifies a waiting thread.
    void push(T value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(value));
        m_cond.notify_one();
    }

    // Waits until an item is available and returns it.
    // Returns std::nullopt if the queue is signaled to close.
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [this] { return !m_queue.empty() || m_closed; });
        
        if (m_closed && m_queue.empty()) {
            return std::nullopt;
        }

        T value = std::move(m_queue.front());
        m_queue.pop();
        return value;
    }

    // Closes the queue, waking up any waiting threads.
    void close() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_closed = true;
        m_cond.notify_all();
    }

private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cond;
    bool m_closed = false;
};

#endif // THREAD_SAFE_QUEUE_HPP