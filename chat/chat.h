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

#include <iostream>
#include <map>
#include "spdlog/spdlog.h"

#ifndef DEFAULT_PORT
#define DEFAULT_PORT "8080"
#endif

int SendMessage(int connection_fd, const char* msg, bool log = true) {
    int msgLen = strlen(msg);
    int flags = 0;
    int bytesSent = send(connection_fd, msg, msgLen, flags);
    if (log)
        spdlog::debug("send: {{msg: '{}', len: {}, bytesSent: {}}}\n", msg, msgLen, bytesSent);
    if (bytesSent < 0)
        perror("send");
    // TODO: handle bytesSent < msgLen by sending another packet
    return bytesSent;
}

int RecvMessage(int connection_fd, char** msg, int recv_flags = 0, bool log = true) {
    static const int bufLen = 2048; // larger than any MTU size
    char buf[bufLen] = {0};

    // TODO: handle multipacket messages
    // for now we just allocate one buffer of memory for the message
    *msg = (char*) malloc(bufLen);

    int bytesReceived = recv(connection_fd, buf, bufLen, recv_flags);
    if (bytesReceived < 0)
        perror("recv");
    else if (bytesReceived == 0) {
        spdlog::info("recv: connection closed by server\n");
    }
    spdlog::debug("recv: {{msg: '{}', len {}}}\n", buf, bytesReceived);

    return bytesReceived;
}


#endif