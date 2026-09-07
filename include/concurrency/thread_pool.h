#pragma once

/// @file thread_pool.h
/// @brief A C++20 thread pool with cooperative cancellation.
///
/// Uses `std::jthread` when the library provides it (`__cpp_lib_jthread`);
/// otherwise `std::thread` plus an explicit join in the destructor (Apple
/// libc++ on some SDKs has no `std::jthread`).

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

#if defined(__cpp_lib_jthread)
#include <stop_token>
#endif

namespace concurrency {

/// Fixed-size thread pool. Enqueue callables and receive std::future results.
class ThreadPool {
  public:
    /// @brief Construct a thread pool with @p n worker threads.
    /// @param n Number of worker threads (defaults to hardware concurrency; minimum 1).
    explicit ThreadPool(unsigned n = std::thread::hardware_concurrency()) {
        const unsigned count = n ? n : 1u;
        workers_.reserve(count);
        for (unsigned i = 0; i < count; ++i) {
#if defined(__cpp_lib_jthread)
            workers_.emplace_back(
                [this](std::stop_token st) { run([&] { return st.stop_requested(); }); });
#else
            workers_.emplace_back([this] { run([&] { return stop_; }); });
#endif
        }
    }

    /// @brief Destroy the pool, draining queued work then joining workers.
    ~ThreadPool() {
#if defined(__cpp_lib_jthread)
        for (auto &w : workers_)
            w.request_stop();
#else
        {
            std::lock_guard lk{mu_};
            stop_ = true;
        }
#endif
        cv_.notify_all();
#if !defined(__cpp_lib_jthread)
        for (auto &w : workers_) {
            if (w.joinable())
                w.join();
        }
#endif
    }

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    /// @brief Submit a callable for asynchronous execution.
    /// @tparam F    Callable type.
    /// @tparam Args Argument types forwarded to @p f.
    /// @param f    The callable to execute.
    /// @param args Arguments forwarded to @p f.
    /// @return A `std::future` holding the eventual result of the callable.
    template <typename F, typename... Args>
    auto submit(F &&f, Args &&...args) -> std::future<std::invoke_result_t<F, Args...>> {
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
    template <typename StopPred> void run(StopPred stop_pred) {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock lk{mu_};
                cv_.wait(lk, [&] { return stop_pred() || !tasks_.empty(); });
                if (tasks_.empty())
                    return; // stop requested; queue drained
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }

    // workers_ must be declared last so joins happen before mu_/cv_/tasks_
    // are destroyed (members are destroyed in reverse declaration order).
    std::queue<std::function<void()>> tasks_;
    std::mutex mu_;
    std::condition_variable cv_;
#if !defined(__cpp_lib_jthread)
    bool stop_{false};
    std::vector<std::thread> workers_;
#else
    std::vector<std::jthread> workers_;
#endif
};

} // namespace concurrency
