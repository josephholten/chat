#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>

#include <docopt/docopt.h>

int main(int, char**){
    const char* server_node = "localhost";
    const char* server_service = "8080";

    // get addr info for server

    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo* server_info = NULL;

    if (int status = getaddrinfo(server_node, server_service, &hints, &server_info); status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return 1;
    };

    int connection_fd = -1;

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

        printf("listening on %s port %d\n", ipstr, port);
        break;
    }
    // don't need server_info any more as we are already listening now
    freeaddrinfo(server_info);

    if (connection_fd == -1) {
        fprintf(stderr, "error: couldn't connect to any ip\n");
        return 1;
    }

    // as client we first receive
    static const int recvBufLen = 1024;
    char recvBuf[recvBufLen];
    int recvFlags = 0;
    if (int bytesReceived = recv(connection_fd, recvBuf, recvBufLen, recvFlags); bytesReceived < 0) {
        perror("recv");
    }
    else if (bytesReceived == 0) {
        printf("recv: connection closed by server\n");
    } else {
        printf("recv: {msg: '%s', len: %d}\n", recvBuf, bytesReceived);
    }

    // then send
    const char *msg = "client connected successfully";
    int msgLen = strlen(msg);
    int flags = 0; // this should be fine
    int bytesSent = send(connection_fd, msg, msgLen, flags);
    printf("send: {msg: '%s', len: %d, bytesSent: %d}\n", msg, bytesSent, msgLen);
    if (bytesSent < 0)
        perror("send");

    close(connection_fd);

    return 0;
}
