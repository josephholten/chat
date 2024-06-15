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

#include <iostream>
#include <map>
#include <format>

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

    const int backlog = 10;

    int listen_fd = ServerListen(
        (args["--host"] ? args["--host"].asString().c_str() : NULL),
        (args["--port"] ? args["--port"].asString().c_str() : DEFAULT_PORT),
        backlog
    );

    if (listen_fd < 0)
        return 1;

    int connection_fd = AcceptConnection(listen_fd);

    // what if both server and client first send? -> no problem, we can even send simultaneously!
    SendMessage(
        connection_fd,
        "server obtained connection"
    );

    char* msg;
    int len = RecvMessage(connection_fd, &msg);
    (void)len;
    free(msg); // got to free msg
    
    close(connection_fd);

    return 0;
}
