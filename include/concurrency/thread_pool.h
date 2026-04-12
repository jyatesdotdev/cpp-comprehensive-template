#pragma once

/// @file thread_pool.h
/// @brief A modern C++20 thread pool with std::jthread and cooperative cancellation.

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

namespace concurrency {

/// Fixed-size thread pool. Enqueue callables and receive std::future results.
/// Uses std::jthread for automatic join on destruction.
class ThreadPool {
public:
    /// @brief Construct a thread pool with @p n worker threads.
    /// @param n Number of worker threads (defaults to hardware concurrency; minimum 1).
    explicit ThreadPool(unsigned n = std::thread::hardware_concurrency()) {
        for (unsigned i = 0; i < (n ? n : 1u); ++i) {
            workers_.emplace_back([this](std::stop_token st) { run(st); });
        }
    }

    /// @brief Destroy the pool, requesting cooperative stop on all workers.
    ~ThreadPool() {
        // Request stop on all jthreads (auto-joined).
        for (auto& w : workers_) w.request_stop();
        cv_.notify_all();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /// @brief Submit a callable for asynchronous execution.
    /// @tparam F    Callable type.
    /// @tparam Args Argument types forwarded to @p f.
    /// @param f    The callable to execute.
    /// @param args Arguments forwarded to @p f.
    /// @return A `std::future` holding the eventual result of the callable.
    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using R = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<R()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        auto fut = task->get_future();
        {
            std::lock_guard lk{mu_};
            tasks_.emplace([task]() { (*task)(); });
        }
        cv_.notify_one();
        return fut;
    }

    /// @brief Return the number of worker threads.
    /// @return Worker thread count.
    [[nodiscard]] std::size_t size() const noexcept { return workers_.size(); }

private:
    void run(std::stop_token st) {
        while (!st.stop_requested()) {
            std::function<void()> task;
            {
                std::unique_lock lk{mu_};
                cv_.wait(lk, [&] { return st.stop_requested() || !tasks_.empty(); });
                if (st.stop_requested() && tasks_.empty()) return;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }

    std::vector<std::jthread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mu_;
    std::condition_variable cv_;
};

} // namespace concurrency
