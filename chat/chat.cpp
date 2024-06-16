#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <format>

#include "docopt/docopt.h"
#include "tcp.h"
#include "chat.h"

static const char USAGE[] = R"(Usage:
    chat <SERVER> [--port=<int>] [--username=<STR>] [--log=LEVEL]
Options:
    -h --help           show this screen
    --port=INT          select port to connect to [Default: 8080]
    -u --username=NAME  set username [Default: anon]
    --log=LEVEL         set logging level, one of error, warn, info, debug
)";

int main(int argc, char** argv){
    std::map<std::string, docopt::value> args = docopt::docopt(
        USAGE,
        { argv + 1, argv + argc }
    );

    if (args["--log"])
        spdlog::set_level(spdlog::level::from_str(args["--log"].asString()));

    std::string username = args["--username"] ? args["--username"].asString() : "anon";

    int connection_fd = -1;
    connection_fd = ClientConnect(args["<SERVER>"].asString().c_str(), (args["--port"] ? args["--port"].asString().c_str() : DEFAULT_PORT));
    if (connection_fd < 0)
        return 1;

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        spdlog::error("epoll_create1: {}", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // listen_fd is ready to read()
    struct epoll_event ev {
        .events = EPOLLIN, // which event to notify on
        .data = { .fd = connection_fd, }, // a "tag" to identify the event
    };
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connection_fd, &ev) < 0) {
        spdlog::error("epoll_ctl: {}", strerror(errno));
        exit(EXIT_FAILURE);
    }
    fcntl(connection_fd, F_SETFL, O_NONBLOCK);

    ev.data.fd = STDIN_FILENO; // stdin
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) < 0) {
        spdlog::error("epoll_ctl: {}", strerror(errno));
        exit(EXIT_FAILURE);
    }

    std::string prompt = ">";

    std::cout << prompt << std::flush;

    static int max_events = 10;
    epoll_event events[max_events];

    for (;;) {
        // miliseconds to max wait (-1 = infty)
        int epoll_timeout = -1;
        int number_of_events = epoll_wait(epoll_fd, events, max_events, epoll_timeout);
        spdlog::debug("{} events", number_of_events);

        if (number_of_events < 0) {
            spdlog::error("epoll_wait: {}", strerror(errno));
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < number_of_events; i++) {
            epoll_event event = events[i];
            if (event.data.fd == connection_fd) {
                spdlog::debug("connection_fd event");
                std::optional<Packet> packet = RecvPacket(connection_fd);
                if (!packet.has_value()) {
                    spdlog::info("server closed connection", packet->username, packet->message);
                    exit(EXIT_SUCCESS);
                } else {
                    // before printing message, save cin
                    std::string line;
                    std::getline(std::cin, line);
                    std::cout << packet->username << ": " << packet->message << std::endl;
                }

            }

            if (event.data.fd == STDIN_FILENO) {
                spdlog::debug("stdin event");
                std::string line;
                std::getline(std::cin, line);
                SendPacket(connection_fd, {.username = username, .message = line});
                std::cout << prompt << std::flush;
            }
        }
    }



    std::cout << ">";
    for (std::string line; std::getline(std::cin, line);) {
        SendPacket(connection_fd, {.username = username, .message = line});
        std::cout << ">";
    }

    close(connection_fd);

    return 0;
}
