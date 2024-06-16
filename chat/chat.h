#ifndef CHAT_HH
#define CHAT_HH

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <iostream>
#include <map>
#include <optional>
#include "spdlog/spdlog.h"

#ifndef DEFAULT_PORT
#define DEFAULT_PORT "8080"
#endif

struct Packet {
    std::string username;
    std::string message;

    void copyToBuf(uint8_t* buf) {
        uint32_t usize = htonl(username.size());
        uint32_t msize = htonl(message.size());
        memcpy(buf, &usize, sizeof(usize));
        memcpy(buf + sizeof(usize), username.c_str(), username.size());
        memcpy(buf + sizeof(usize) + username.size(), &msize, sizeof(msize));
        memcpy(buf + sizeof(usize) + username.size() + sizeof(msize), message.c_str(), message.size());
    }

    uint32_t frame_length() {
        return (uint32_t)(sizeof(uint32_t) + sizeof(uint32_t) + username.size() + sizeof(uint32_t) + message.size());
    }
};


bool SendPacket(int connection_fd, Packet packet) {
    int flags = 0;
    uint8_t* buf = (uint8_t*)malloc(packet.frame_length());
    uint32_t frame_length_n = htonl(packet.frame_length());
    memcpy(buf, &frame_length_n, sizeof(frame_length_n));
    packet.copyToBuf(buf+sizeof(frame_length_n));

    int bytes_sent = 0;
    while (bytes_sent < packet.frame_length()) {
        int bytes = send(connection_fd, buf + bytes_sent, packet.frame_length(), flags);
        if (bytes < 0) {
            spdlog::error("send: {}", strerror(errno));
            return false;
        }
        if (bytes == 0) {
            spdlog::error("send: sent zero bytes, trying again", strerror(errno));
        }
        bytes_sent += bytes;
    }
    return true;
}

uint32_t DeserializeU32(uint8_t* buf) {
    uint32_t res;
    res |= buf[0] << 24;
    res |= buf[1] << 16;
    res |= buf[2] << 8;
    res |= buf[3];
    return res;
}

std::string DeserializeString(uint8_t* buf) {
    uint32_t size = DeserializeU32(buf);
    return std::string(
        (char*)(buf+sizeof(size)),
        (char*)(buf+sizeof(size)+size)
    );
}

std::optional<Packet> RecvPacket(int connection_fd, int recv_flags = 0) {
    static const int tempBufLen = 2048; // larger than any MTU size
    uint8_t tempBuf[tempBufLen] = {0};

    int bytes_received = recv(connection_fd, tempBuf, tempBufLen, recv_flags);
    if (bytes_received < 0) {
        spdlog::error("recv: {}", strerror(errno));
        return {};
    } else if (bytes_received == 0) {
        spdlog::info("recv: connection closed by remote");
        return {};
    } else if (bytes_received < 4) {
        spdlog::error("recv: too few bytes");
    }

    uint32_t frame_length = DeserializeU32(tempBuf);
    uint8_t* frameBuf = (uint8_t*)malloc(frame_length);
    memcpy(frameBuf, tempBuf, bytes_received);

    while(bytes_received < frame_length) {
        int bytes = recv(connection_fd, frameBuf + bytes_received, frame_length - bytes_received, recv_flags);
        if (bytes < 0) {
            spdlog::error("recv: {}", strerror(errno));
            return {};
        } else if (bytes == 0) {
            spdlog::info("recv: connection closed by remote");
            return {};
        }
        bytes_received += bytes;
    }

    Packet packet;
    packet.username = DeserializeString(frameBuf+sizeof(frame_length));
    packet.message = DeserializeString(frameBuf+sizeof(frame_length)+sizeof(uint32_t)+packet.username.size());

    return packet;
}


#endif