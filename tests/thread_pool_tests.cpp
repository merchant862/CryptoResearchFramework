#include "core/utils/thread_pool.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <future>
#include <vector>

namespace {

TEST(ThreadPool, RunsSubmittedWorkAndReturnsItsResult) {
    crypto_research::utils::ThreadPool pool{2};
    auto first = pool.submit([] { return 21; });
    auto second = pool.submit([] { return 21; });
    EXPECT_EQ(first.get() + second.get(), 42);
}

TEST(ThreadPool, ProcessesAllQueuedTasks) {
    crypto_research::utils::ThreadPool pool{3};
    std::atomic<int> completed{};
    std::vector<std::future<void>> futures;
    for (int index = 0; index < 32; ++index) futures.push_back(pool.submit([&completed] { completed.fetch_add(1); }));
    for (auto& future : futures) future.get();
    EXPECT_EQ(completed.load(), 32);
}

} // namespace
