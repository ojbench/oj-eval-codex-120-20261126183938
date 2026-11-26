#include "src.hpp"
#include <algorithm>
#include <cassert>
#include <random>

int main() {
    Timer timer;
    struct Entry {
        Task* task;
        TaskNode* node;
        size_t due, period;
    };
    std::vector<Entry> entries;
    std::mt19937 random(2560);
    const size_t delays[] = {1, 59, 60, 61, 3599, 3600, 3601, 80000};
    for (size_t now = 0; now < 250000; ++now) {
        if (now % 7 == 0 && entries.size() < 500) {
            size_t delay = delays[random() % 8];
            size_t period = delays[random() % 8];
            Task* task = new Task("test", delay, period);
            entries.push_back({task, timer.addTask(task), now + delay, period});
        }
        if (now % 13 == 0 && !entries.empty()) {
            size_t index = random() % entries.size();
            timer.cancelTask(entries[index].node);
            delete entries[index].task;
            entries.erase(entries.begin() + index);
        }
        auto actual = timer.tick();
        size_t expected = 0;
        for (auto& entry : entries) {
            if (entry.due == now + 1) {
                ++expected;
                assert(std::count(actual.begin(), actual.end(), entry.task) == 1);
                entry.due += entry.period;
            }
        }
        assert(actual.size() == expected);
    }
    for (auto& entry : entries) {
        timer.cancelTask(entry.node);
        delete entry.task;
    }
    Task outside("outside", 90000, 1);
    Timer fresh;
    assert(fresh.addTask(&outside) == nullptr);
    Task once("once", 1, 90000);
    fresh.addTask(&once);
    assert(fresh.tick().size() == 1);
    assert(fresh.tick().empty());
    Task full_day("day", 86400, 86400);
    fresh.addTask(&full_day);
    for (int i = 1; i <= 172800; ++i) {
        assert(fresh.tick().size() == (i % 86400 == 0 ? 1u : 0u));
    }
}
