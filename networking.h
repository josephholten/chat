#ifndef NETWORKING_HH
#define NETWORKING_HH

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <vector>

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

std::string SockaddrToString(sockaddr_storage* sa) {
    //char ipstr[INET6_ADDRSTRLEN];
    std::vector<char> ipstr(INET6_ADDRSTRLEN);
    inet_ntop(sa->ss_family, GetInAddr((sockaddr*)sa), ipstr.data(), ipstr.size());
    return std::string(ipstr.begin(), ipstr.end());
}

#endif