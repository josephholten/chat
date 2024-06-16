#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <format>
#include <unordered_set>

#include "spdlog/spdlog.h"
#include "docopt/docopt.h"
#include "tcp.h"
#include "chat.h"

static const char USAGE[] = R"(Usage:
    chatd [--host=IP] [--port=INT] [--log=LEVEL]
Options:
    -h --help   show this screen
    --host=IP   host IP to bind to [Default: 0.0.0.0]
    --port=INT  select port to connect to [Default: 8080]
    --log=LEVEL how much logging to print, LEVEL is one of ERROR, WARN, INFO, DEBUG
)";

int main(int argc, char** argv){
    std::map<std::string, docopt::value> args = docopt::docopt(
        USAGE,
        { argv + 1, argv + argc }
    );

    if (args["--log"])
        spdlog::set_level(spdlog::level::from_str(args["--log"].asString()));

    static int backlog = 10;

    int listen_fd = ServerListen(
        (args["--host"] ? args["--host"].asString().c_str() : NULL),
        (args["--port"] ? args["--port"].asString().c_str() : DEFAULT_PORT),
        backlog
    );

    if (listen_fd < 0)
        exit(EXIT_FAILURE);

    fcntl(listen_fd, F_SETFL, O_NONBLOCK);

    // should probably use libevent instead
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        spdlog::error("epoll_create1: {}", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // listen_fd is ready to read()
    struct epoll_event ev {
        .events = EPOLLIN, // which event to notify on
        .data = { .fd = listen_fd, }, // a "tag" to identify the event
    };

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
        spdlog::error("epoll_ctl: {}", strerror(errno));
        exit(EXIT_FAILURE);
    }

    static int max_events = 10;
    epoll_event events[max_events];

    std::unordered_set<int> connections;

    for (;;) {
        // miliseconds to max wait (-1 = infty)
        int epoll_timeout = -1;
        int number_of_events = epoll_wait(epoll_fd, events, max_events, epoll_timeout);

        if (number_of_events < 0) {
            spdlog::error("epoll_wait: {}", strerror(errno));
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < number_of_events; i++) {
            epoll_event event = events[i];
            if (event.data.fd == listen_fd) {
                int connection_fd = AcceptConnection(listen_fd);
                fcntl(connection_fd, F_SETFL, O_NONBLOCK);
                epoll_event ev {
                    .events = EPOLLIN,
                    .data = { .fd = connection_fd },
                };
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connection_fd, &ev) == -1) {
                    spdlog::error("trying to add connection_fd {} to epoll events", connection_fd);
                    spdlog::error("epoll_ctl: {}", strerror(errno));
                    spdlog::error("will ignore this connection");
                    close(connection_fd);
                }
                connections.insert(connection_fd);
            }

            else {
                int connection_fd = event.data.fd;
                std::optional<Packet> packet = RecvPacket(connection_fd);
                if (!packet.has_value()) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connection_fd, NULL);
                    close(connection_fd);
                    connections.erase(connection_fd);
                } else {
                    spdlog::info("{}: {}", packet->username, packet->message);
                    // broadcast packet
                    for (int fd : connections) {
                        // but don't send it back
                        if (fd != connection_fd)
                            SendPacket(fd, *packet);
                    }
                }
            }
        }
    }

    close(epoll_fd);
    close(listen_fd);

    return 0;
}
