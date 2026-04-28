#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <queue>
#include <string>

struct Task {
    int priority;
    std::string description;

    bool operator<(const Task& other) const {
        return priority > other.priority;
    }
};

class SJFScheduler {
private:
    std::priority_queue<Task> tasks;

public:
    void addTask(int priority, const std::string& description) {
        tasks.push({priority, description});
    }

    bool hasTask() const {
        return !tasks.empty();
    }

    Task getNextTask() {
        Task next = tasks.top();
        tasks.pop();
        return next;
    }
};

#endif
