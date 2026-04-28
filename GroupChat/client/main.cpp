#include <iostream>
#include <thread>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "../shared/protocol.h"

void receiveMessages(int sock) {
    char buffer[1024];

    while (true) {
        int bytes = read(sock, buffer, sizeof(buffer) - 1);

        if (bytes <= 0) {
            break;
        }

        buffer[bytes] = '\0';
        std::cout << "\n" << buffer << "> ";
        std::cout.flush();
    }
}

int main() {
    int sock = 0;
    sockaddr_in servAddr{};

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        std::cout << "Socket creation error\n";
        return 1;
    }

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr) <= 0) {
        std::cout << "Invalid address\n";
        return 1;
    }

    if (connect(sock, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        std::cout << "Connection failed\n";
        return 1;
    }

    std::thread receiver(receiveMessages, sock);
    receiver.detach();

    std::cout << "Commands:\n";
    std::cout << "/join groupID\n";
    std::cout << "/create groupID\n";
    std::cout << "/switch groupID\n";
    std::cout << "/list\n";
    std::cout << "/quit\n";
    std::cout << "Anything else sends a message.\n";

    std::string input;

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);

        ChatPacket packet{};

        if (input.rfind("/join ", 0) == 0) {
            int groupID = std::stoi(input.substr(6));
            packet = makePacket(JOIN_GROUP, groupID, "");
        }

        else if (input.rfind("/create ", 0) == 0) {
            int groupID = std::stoi(input.substr(8));
            packet = makePacket(CREATE_GROUP, groupID, "");
        }

        else if (input.rfind("/switch ", 0) == 0) {
            int groupID = std::stoi(input.substr(8));
            packet = makePacket(SWITCH_GROUP, groupID, "");
        }

        else if (input == "/list") {
            packet = makePacket(LIST_GROUPS, 0, "");
        }

        else if (input == "/quit") {
            packet = makePacket(DISCONNECT, 0, "");
            send(sock, &packet, sizeof(packet), 0);
            break;
        }

        else {
            packet = makePacket(SEND_MESSAGE, 0, input);
        }

        send(sock, &packet, sizeof(packet), 0);
    }

    close(sock);
    return 0;
}
