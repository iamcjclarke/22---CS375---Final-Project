#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "../shared/protocol.h"
#include "../shared/cache.h"
#include "../shared/thread_pool.h"
#include "../shared/scheduler.h"

std::mutex serverMutex;
std::map<int, std::set<int>> groups;
std::map<int, int> clientGroups;
MessageCache messageCache(5);
SJFScheduler scheduler;

void logMessage(const std::string& msg) {
    std::lock_guard<std::mutex> lock(serverMutex);
    std::ofstream logFile("logs/chat_log.txt", std::ios::app);
    logFile << msg << std::endl;
}

void sendText(int clientSocket, const std::string& msg) {
    send(clientSocket, msg.c_str(), msg.size(), 0);
}

void broadcastToGroup(int groupID, const std::string& message, int senderSocket) {
    std::lock_guard<std::mutex> lock(serverMutex);

    for (int client : groups[groupID]) {
        if (client != senderSocket) {
            send(client, message.c_str(), message.size(), 0);
        }
    }
}

void handleClient(int clientSocket) {
    int currentGroup = 1;

    {
        std::lock_guard<std::mutex> lock(serverMutex);
        groups[currentGroup].insert(clientSocket);
        clientGroups[clientSocket] = currentGroup;
    }

    sendText(clientSocket, "Connected to server. Default group: 1\n");

    ChatPacket packet{};

    while (true) {
        int bytesRead = read(clientSocket, &packet, sizeof(packet));

        if (bytesRead <= 0) {
            break;
        }

        int groupID = getGroupID(packet);
        std::string payload = packet.payload;

        if (packet.type == CREATE_GROUP || packet.type == JOIN_GROUP || packet.type == SWITCH_GROUP) {
            std::lock_guard<std::mutex> lock(serverMutex);

            groups[currentGroup].erase(clientSocket);
            currentGroup = groupID;
            groups[currentGroup].insert(clientSocket);
            clientGroups[clientSocket] = currentGroup;

            sendText(clientSocket, "Switched/joined group " + std::to_string(currentGroup) + "\n");

            auto recent = messageCache.getRecentMessages(currentGroup);
            for (const auto& msg : recent) {
                sendText(clientSocket, "[Recent] " + msg + "\n");
            }
        }

        else if (packet.type == LIST_GROUPS) {
            std::lock_guard<std::mutex> lock(serverMutex);
            std::string list = "Active groups: ";

            for (auto& group : groups) {
                list += std::to_string(group.first) + " ";
            }

            list += "\n";
            sendText(clientSocket, list);
        }

        else if (packet.type == SEND_MESSAGE) {
            scheduler.addTask(1, "Message task");

            std::string fullMessage =
                "[Group " + std::to_string(currentGroup) + "] Client " +
                std::to_string(clientSocket) + ": " + payload;

            {
                std::lock_guard<std::mutex> lock(serverMutex);
                messageCache.addMessage(currentGroup, fullMessage);
            }

            logMessage(fullMessage);
            broadcastToGroup(currentGroup, fullMessage + "\n", clientSocket);
            sendText(clientSocket, "Message sent.\n");
        }

        else if (packet.type == DISCONNECT) {
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(serverMutex);
        groups[currentGroup].erase(clientSocket);
        clientGroups.erase(clientSocket);
    }

    close(clientSocket);
    logMessage("Client disconnected: " + std::to_string(clientSocket));
}

int main() {
    int serverFD;
    int newSocket;
    sockaddr_in address{};
    int addrlen = sizeof(address);

    mkdir("logs", 0777);

    serverFD = socket(AF_INET, SOCK_STREAM, 0);

    if (serverFD == 0) {
        perror("Socket failed");
        return 1;
    }

    int opt = 1;
    setsockopt(serverFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(serverFD, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }

    if (listen(serverFD, 10) < 0) {
        perror("Listen failed");
        return 1;
    }

    ThreadPool pool(4);

    std::cout << "Server running on port 8080..." << std::endl;

    while (true) {
        newSocket = accept(serverFD, (struct sockaddr*)&address, (socklen_t*)&addrlen);

        if (newSocket < 0) {
            perror("Accept failed");
            continue;
        }

        std::cout << "New client connected: " << newSocket << std::endl;

        pool.enqueue([newSocket]() {
            handleClient(newSocket);
        });
    }

    close(serverFD);
    return 0;
}
