#include "core/utils/thread_pool.hpp"

#include <algorithm>

namespace crypto_research::utils {

ThreadPool::ThreadPool(std::size_t worker_count) {
    worker_count = std::max<std::size_t>(1, worker_count);
    workers_.reserve(worker_count);
    for (std::size_t index = 0; index < worker_count; ++index) workers_.emplace_back([this] { worker_loop(); });
}

ThreadPool::~ThreadPool() {
    {
        std::scoped_lock lock{mutex_};
        stopping_ = true;
    }
    work_available_.notify_all();
    for (auto& worker : workers_) worker.join();
}

void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock lock{mutex_};
            work_available_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });
            if (stopping_ && tasks_.empty()) return;
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
}

} // namespace crypto_research::utils
