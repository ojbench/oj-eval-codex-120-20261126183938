#pragma once

#include "Task.hpp"
#include <vector>

class TaskNode {
    friend class TimingWheel;
    friend class Timer;

private:
    Task* task;
    TaskNode* next = nullptr;
    TaskNode* prev = nullptr;
    TaskNode** head = nullptr;
    size_t time;

    explicit TaskNode(Task* value) : task(value), time(0) {}
};

class TimingWheel {
    friend class Timer;

public:
    TimingWheel(size_t size, size_t interval)
        : size(size), interval(interval), slots(new TaskNode*[size]()) {}

    ~TimingWheel() {
        for (size_t i = 0; i < size; ++i) {
            TaskNode* node = slots[i];
            while (node) {
                TaskNode* next = node->next;
                delete node;
                node = next;
            }
        }
        delete[] slots;
    }

    TimingWheel(const TimingWheel&) = delete;
    TimingWheel& operator=(const TimingWheel&) = delete;

private:
    const size_t size, interval;
    size_t current_slot = 0;
    TaskNode** slots;

    void insert(TaskNode* node, size_t time) {
        const size_t slot = (current_slot + time / interval) % size;
        node->time = time;
        node->head = &slots[slot];
        node->prev = nullptr;
        node->next = slots[slot];
        if (node->next) node->next->prev = node;
        slots[slot] = node;
    }

    TaskNode* takeCurrent() {
        TaskNode* result = slots[current_slot];
        slots[current_slot] = nullptr;
        return result;
    }

    bool advance() {
        current_slot = (current_slot + 1) % size;
        return current_slot == 0;
    }
};

class Timer {
public:
    Timer() : seconds(60, 1), minutes(60, 60), hours(24, 3600) {}
    ~Timer() = default;
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

    TaskNode* addTask(Task* task) {
        TaskNode* node = new TaskNode(task);
        return schedule(node, task->getFirstInterval()) ? node : nullptr;
    }

    void cancelTask(TaskNode* node) {
        if (!node) return;
        if (node->prev) node->prev->next = node->next;
        else *node->head = node->next;
        if (node->next) node->next->prev = node->prev;
        delete node;
    }

    std::vector<Task*> tick() {
        // Cascade from highest to lowest so exact-boundary tasks fire now.
        if (seconds.advance()) {
            if (minutes.advance()) {
                hours.advance();
                cascade(hours, minutes);
            }
            cascade(minutes, seconds);
        }
        std::vector<Task*> ready;
        TaskNode* node = seconds.takeCurrent();
        while (node) {
            TaskNode* next = node->next;
            ready.push_back(node->task);
            schedule(node, node->task->getPeriod());
            node = next;
        }
        return ready;
    }

private:
    TimingWheel seconds, minutes, hours;

    bool schedule(TaskNode* node, size_t time) {
        TimingWheel* wheels[] = {&seconds, &minutes, &hours};
        for (TimingWheel* wheel : wheels) {
            if (time / wheel->interval <= wheel->size) {
                wheel->insert(node, time);
                return true;
            }
            const size_t offset = wheel->current_slot * wheel->interval;
            // Reject an unrepresentable delay before adding the lower offset.
            if (time > static_cast<size_t>(-1) - offset) break;
            time += offset;
        }
        delete node;
        return false;
    }

    static void cascade(TimingWheel& upper, TimingWheel& lower) {
        TaskNode* node = upper.takeCurrent();
        while (node) {
            TaskNode* next = node->next;
            lower.insert(node, node->time % upper.interval);
            node = next;
        }
    }
};
