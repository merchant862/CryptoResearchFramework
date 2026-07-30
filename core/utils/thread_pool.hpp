#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace crypto_research::utils {

class ThreadPool final {
public:
    explicit ThreadPool(std::size_t worker_count = std::thread::hardware_concurrency());
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename Function>
    auto submit(Function&& function) -> std::future<std::invoke_result_t<Function>> {
        using Result = std::invoke_result_t<Function>;
        auto task = std::make_shared<std::packaged_task<Result()>>(std::forward<Function>(function));
        auto future = task->get_future();
        {
            std::scoped_lock lock{mutex_};
            if (stopping_) throw std::runtime_error("cannot submit work to a stopped thread pool");
            tasks_.emplace([task] { (*task)(); });
        }
        work_available_.notify_one();
        return future;
    }

    [[nodiscard]] std::size_t worker_count() const noexcept { return workers_.size(); }

private:
    void worker_loop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable work_available_;
    bool stopping_ = false;
};

} // namespace crypto_research::utils
