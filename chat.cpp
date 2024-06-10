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

void GetAddrPort(struct addrinfo *p, void **addr, int *port)
{
    if (p->ai_family == AF_INET) { // IPv4
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        *addr = &(ipv4->sin_addr);
        *port = ntohs(ipv4->sin_port);
    } else { // IPv6
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
        *addr = &(ipv6->sin6_addr);
        *port = ntohs(ipv6->sin6_port);
    }
}

// get internet address of sockaddr object for IPv4 or IPv6
void* GetInAddr(struct sockaddr* sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    } else {
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
    }
}

/* 
 * listen to incoming connection requests 
 * @param node      url or ip 
 * @param service   either plain text (http, ftp, ...) or port number as string  
 * @param backlog   size of connection queue before refusing connection requests
 * @returns socket file descriptor of the listening socket; use as input to accept
 */
int ServerListen(const char* node, const char* service, int backlog) {
    struct addrinfo hints = {
        .ai_flags = AI_PASSIVE,
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo* server_info = NULL;

    if (int status = getaddrinfo(node, service, &hints, &server_info); status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return 1;
    };

    int listen_fd = -1;
    for (struct addrinfo* p = server_info; p != NULL; p = p->ai_next) {
        void* addr;
        int port;

        GetAddrPort(p, &addr, &port);

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

        printf("listening on socket %d bound to %s :%d\n", listen_fd, ipstr, port);
        break;
    }
    // don't need server_info any more as we are already listening now
    freeaddrinfo(server_info);

    if (listen_fd == -1)
        fprintf(stderr, "error: couldn't listen on any ip\n");

    return listen_fd;
}

/* 
 * connect to remote socket
 * @param node      url or ip 
 * @param service   either plain text (http, ftp, ...) or port number as string  
 * @returns socket file descriptor of the listening socket; use as input to accept
 */
int ClientConnect(const char* node, const char* service) {
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo* server_info = NULL;

    if (int status = getaddrinfo(node, service, &hints, &server_info); status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return 1;
    };

    int connection_fd = -1;

    for (struct addrinfo* p = server_info; p != NULL; p = p->ai_next) {
        void* addr;
        int port;

        GetAddrPort(p, &addr, &port);

        char ipstr[INET6_ADDRSTRLEN];
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);

        connection_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (connection_fd == -1) {
            fprintf(stderr, "error: trying to open socket for %s :%d\n", ipstr, port);
            perror("socket");
            continue;
        }

        if (int res = connect(connection_fd, p->ai_addr, p->ai_addrlen); res < 0) {
            fprintf(stderr, "error: trying to connect to %s :%d\n", ipstr, port);
            perror("connect");
            continue;
        }

        printf("connected to on %s port %d\n", ipstr, port);
        break;
    }
    // don't need server_info any more as we are already listening now
    freeaddrinfo(server_info);

    if (connection_fd == -1) {
        fprintf(stderr, "error: couldn't connect to any ip\n");
        return 1;
    }

    return connection_fd;
}

int SendMessage(int connection_fd, const char* msg, bool log = true) {
    int msgLen = strlen(msg);
    int flags = 0;
    int bytesSent = send(connection_fd, msg, msgLen, flags);
    if (log)
        printf("send: {msg: '%s', len: %d, bytesSent: %d}\n", msg, msgLen, bytesSent);
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
        printf("recv: connection closed by server\n");
    } else if (log) {
        printf("recv: {msg: '%s', len %d}\n", buf, bytesReceived);
    }

    return bytesReceived;
}

#ifndef DEFAULT_PORT
#define DEFAULT_PORT "8080"
#endif

static const char USAGE[] = R"(Usage:
    chat server [--host=<IP>] [--port=<int>]
    chat client <SERVER> [--port=<int>]
Options:
    -h --help     show this screen
    --host=<IP>   host IP to bind to [Default: 0.0.0.0]
    --port=<int>  select port to connect to [Default: 8080]
)";

int main(int argc, char** argv){
    std::map<std::string, docopt::value> args = docopt::docopt(
        USAGE,
        { argv + 1, argv + argc }
    );

    int connection_fd = -1;
    if (args["server"].asBool()) {
        const int backlog = 10;

        int listen_fd = ServerListen(
            (args["--host"] ? args["--host"].asString().c_str() : NULL),
            (args["--port"] ? args["--port"].asString().c_str() : DEFAULT_PORT),
            backlog
        );

        if (listen_fd < 0)
            return 1;

        struct sockaddr_storage incoming_addr;
        socklen_t incoming_addr_size = sizeof incoming_addr;
        // TODO: handle multiple connections here
        connection_fd = accept(listen_fd, (struct sockaddr *)&incoming_addr, &incoming_addr_size);
        if (connection_fd < 0) {
            perror("accept");
        }

        char ipstr[INET6_ADDRSTRLEN];
        inet_ntop(incoming_addr.ss_family, GetInAddr((struct sockaddr*)&incoming_addr), ipstr, sizeof ipstr);
        printf("accepted connection from %s -> connection_fd %d\n", ipstr, connection_fd);

    } else {
        connection_fd = ClientConnect(args["<SERVER>"].asString().c_str(), (args["--port"] ? args["--port"].asString().c_str() : DEFAULT_PORT));
        if (connection_fd < 0)
            return 1;
    }

    // what if both server and client first send? -> no problem, we can even send simultaneously!
    SendMessage(
        connection_fd,
        (args["server"].asBool() ? "server obtained connection" : "client connected successfully")
    );
    char* msg;
    int len = RecvMessage(connection_fd, &msg);
    (void)len;
    free(msg); // got to free msg
    
    close(connection_fd);

    return 0;
}
