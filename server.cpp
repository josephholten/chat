#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv){
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
        printf("trying to listen on %s port %d\n", ipstr, port);

        listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listen_fd == -1) {
            perror("socket");
            continue;
        }

        if (int res = bind(listen_fd, p->ai_addr, p->ai_addrlen); res == -1) {
            perror("bind");
            continue;
        }

        if (int res = listen(listen_fd, backlog); res < 0) {
            perror("listen");
            continue;
        }

        printf("listen successful\n");
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
    int msg_len = strlen(msg);
    int flags = 0; // this should be fine

    printf("sending msg '%s'\n", msg, msg_len);
    int bytes_sent = send(connection_fd, msg, msg_len, flags);
    if (bytes_sent < 0)
        perror("send");
    else
        printf("sent %d/%d bytes\n", bytes_sent, msg_len);

    // then receive
    int recvBufLen = 1024;
    char* recvBuf[recvBufLen];
    int recvFlags = 0;
    if (int bytesReceived = recv(connection_fd, recvBuf, recvBufLen, recvFlags); bytesReceived < 0) {
        perror("recv");
    }
    else if (bytesReceived == 0) {
        printf("recv: connection closed by server\n");
    } else {
        printf("recv: '%s' (len %d)\n", recvBuf, bytesReceived);
    }

    close(connection_fd);

    return 0;
}
