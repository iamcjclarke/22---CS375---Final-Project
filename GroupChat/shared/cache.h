#ifndef CACHE_H
#define CACHE_H

#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

class MessageCache {
private:
    size_t maxSize;
    std::unordered_map<int, std::deque<std::string>> cache;

public:
    MessageCache(size_t size = 5) : maxSize(size) {}

    void addMessage(int groupID, const std::string& message) {
        auto& messages = cache[groupID];

        if (messages.size() >= maxSize) {
            messages.pop_front();
        }

        messages.push_back(message);
    }

    std::vector<std::string> getRecentMessages(int groupID) {
        std::vector<std::string> result;

        for (const auto& msg : cache[groupID]) {
            result.push_back(msg);
        }

        return result;
    }
};

#endif
