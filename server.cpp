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

#include "docopt.cpp/docopt.h"

static const char USAGE[] = R"(Usage:
    chat server [--host=<IP>] [--port=<int>]
    chat client <SERVER_URL> [--port=<int>]
Options:
    -h --help     show this screen
    --hort=<IP>   host IP to bind to [Default: 0.0.0.0]
    --port=<int>  select port to connect to [Default: 8080]
)";

int main(int argc, char** argv){
    std::map<std::string, docopt::value> args = docopt::docopt(
        USAGE,
        { argv + 1, argv + argc }
    );

    const char* server_node = NULL;
    const char* server_service = "8080";
    const int backlog = 10;

    // setup for server listening to incoming requests

    struct addrinfo hints = {
        .ai_flags = AI_PASSIVE,
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo* server_info = NULL;

    if (int status = getaddrinfo(server_node, server_service, &hints, &server_info); status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return 1;
    };

    int listen_fd = -1;
    for (struct addrinfo* p = server_info; p != NULL; p = p->ai_next) {
        void* addr;
        int port;

        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
            addr = &(ipv4->sin_addr);
            port = ntohs(ipv4->sin_port);
        } else { // IPv6
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            port = ntohs(ipv6->sin6_port);
        }

        char ipstr[INET6_ADDRSTRLEN];
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);

        listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listen_fd == -1) {
            fprintf(stderr, "error: trying to open socket for %s :%d\n", ipstr, port);
            perror("socket");
            listen_fd = -1;
            continue;
        }

        if (int res = bind(listen_fd, p->ai_addr, p->ai_addrlen); res == -1) {
            fprintf(stderr, "error: trying to bind socket to %s :%d\n", ipstr, port);
            perror("bind");
            listen_fd = -1;
            continue;
        }

        if (int res = listen(listen_fd, backlog); res < 0) {
            fprintf(stderr, "error: trying to listen to socket bound to %s :%d\n", ipstr, port);
            perror("listen");
            listen_fd = -1;
            continue;
        }

        printf("listening on socket %d bound to %s port %d\n", listen_fd, ipstr, port);
        break;
    }
    // don't need server_info any more as we are already listening now
    freeaddrinfo(server_info);

    if (listen_fd == -1) {
        fprintf(stderr, "error: couldn't listen on any ip\n");
        return 1;
    }

    struct sockaddr_storage incoming_addr;
    socklen_t incoming_addr_size = sizeof incoming_addr;

    int connection_fd = accept(listen_fd, (struct sockaddr *)&incoming_addr, &incoming_addr_size);
    if (connection_fd < 0)
    {
        perror("accept");
    }

    printf("accepted connection with connection_fd %d\n", connection_fd);

    // as server we first send
    const char *msg = "server obtained connection";
    int msgLen = strlen(msg);
    int flags = 0; // this should be fine
    int bytesSent = send(connection_fd, msg, msgLen, flags);
    printf("send: {msg: '%s', len: %d, bytesSent: %d}\n", msg, bytesSent, msgLen);
    if (bytesSent < 0)
        perror("send");

    // then receive
    static const int recvBufLen = 1024;
    char recvBuf[recvBufLen];
    int recvFlags = 0;
    int bytesReceived = recv(connection_fd, recvBuf, recvBufLen, recvFlags);
    if (bytesReceived < 0)
        perror("recv");
    else if (bytesReceived == 0) {
        printf("recv: connection closed by server\n");
    } else {
        printf("recv: {msg: '%s', len %d}\n", recvBuf, bytesReceived);
    }

    close(connection_fd);

    return 0;
}
