#ifndef TCP_HH
#define TCP_HH

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>

#include "spdlog/spdlog.h"
#include "networking.h"

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
        spdlog::error("getaddrinfo error: {}", gai_strerror(status));
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
            spdlog::debug("trying to open socket for {} :{}", ipstr, port);
            spdlog::debug("socket: {}", strerror(errno));
            listen_fd = -1;
            continue;
        }

        if (int res = bind(listen_fd, p->ai_addr, p->ai_addrlen); res == -1) {
            spdlog::debug("trying to bind socket to {} :{}", ipstr, port);
            spdlog::debug("bind: {}", strerror(errno));
            listen_fd = -1;
            continue;
        }

        if (int res = listen(listen_fd, backlog); res < 0) {
            spdlog::debug("trying to listen to socket bound to {} :{}", ipstr, port);
            spdlog::debug("listen: {}", strerror(errno));
            listen_fd = -1;
            continue;
        }

        spdlog::info("listening on socket {} bound to {} :{}", listen_fd, ipstr, port);
        break;
    }
    // don't need server_info any more as we are already listening now
    freeaddrinfo(server_info);

    if (listen_fd == -1)
        spdlog::error("error: couldn't listen on any ip");

    return listen_fd;
}

int AcceptConnection(int listen_fd) {
    sockaddr_storage incoming_addr;
    socklen_t incoming_addr_size = sizeof incoming_addr;
    // TODO: handle multiple connections here
    int connection_fd = accept(listen_fd, (struct sockaddr *)&incoming_addr, &incoming_addr_size);
    if (connection_fd < 0) {
        spdlog::error("accept: {}", strerror(errno));
    }

    spdlog::info("accepted connection from {} -> connection_fd {}", SockaddrToString(&incoming_addr), connection_fd);

    return connection_fd;
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
        spdlog::error("getaddrinfo error: {}", gai_strerror(status));
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
            spdlog::debug("trying to open socket for {} :{}", ipstr, port);
            spdlog::debug("socket: {}", strerror(errno));
            continue;
        }

        if (int res = connect(connection_fd, p->ai_addr, p->ai_addrlen); res < 0) {
            spdlog::debug("trying to connect to {} :{}", ipstr, port);
            spdlog::debug("connect: {}", strerror(errno));
            continue;
        }

        spdlog::info("connected to {} port {}", ipstr, port);
        break;
    }
    // don't need server_info any more as we are already listening now
    freeaddrinfo(server_info);

    if (connection_fd == -1) {
        spdlog::error("couldn't connect to any ip");
        return 1;
    }

    return connection_fd;
}

#endif