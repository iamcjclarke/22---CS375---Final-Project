#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <cstring>
#include <string>
#include <arpa/inet.h>

const int PAYLOAD_SIZE = 256;

enum PacketType {
    JOIN_GROUP = 1,
    CREATE_GROUP = 2,
    SEND_MESSAGE = 3,
    SWITCH_GROUP = 4,
    LIST_GROUPS = 5,
    DISCONNECT = 6
};

struct ChatPacket {
    uint8_t type;
    uint16_t groupID;
    uint32_t timestamp;
    char payload[PAYLOAD_SIZE];
};

inline ChatPacket makePacket(uint8_t type, uint16_t groupID, const std::string& msg) {
    ChatPacket packet{};
    packet.type = type;
    packet.groupID = htons(groupID);
    packet.timestamp = htonl(static_cast<uint32_t>(time(nullptr)));
    std::strncpy(packet.payload, msg.c_str(), PAYLOAD_SIZE - 1);
    return packet;
}

inline uint16_t getGroupID(const ChatPacket& packet) {
    return ntohs(packet.groupID);
}

inline uint32_t getTimestamp(const ChatPacket& packet) {
    return ntohl(packet.timestamp);
}

#endif
