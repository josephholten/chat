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

    int connection_fd = -1;
    connection_fd = ClientConnect(args["<SERVER>"].asString().c_str(), (args["--port"] ? args["--port"].asString().c_str() : DEFAULT_PORT));
    if (connection_fd < 0)
        return 1;

    char* msg;
    int len = RecvMessage(connection_fd, &msg);
    (void)len;
    free(msg); // got to free msg

    // what if both server and client first send? -> no problem, we can even send simultaneously!
    SendMessage(
        connection_fd,
        std::format("{} connected successfully", (args["--username"] ? args["--username"].asString().c_str() : "anon"))
    );
    close(connection_fd);

    return 0;
}
