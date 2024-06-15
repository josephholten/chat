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

#include "docopt/docopt.h"
#include "tcp.h"
#include "chat.h"

static const char USAGE[] = R"(Usage:
    chat <SERVER> [--port=<int>] [--username=<STR>]
Options:
    -h --help            show this screen
    --port=<int>         select port to connect to [Default: 8080]
    -u --username=<NAME> set username [Default: anon]
)";

int main(int argc, char** argv){
    std::map<std::string, docopt::value> args = docopt::docopt(
        USAGE,
        { argv + 1, argv + argc }
    );

    int connection_fd = -1;
    connection_fd = ClientConnect(args["<SERVER>"].asString().c_str(), (args["--port"] ? args["--port"].asString().c_str() : DEFAULT_PORT));
    if (connection_fd < 0)
        return 1;

    char* msg;
    int len = RecvMessage(connection_fd, &msg);
    (void)len;
    free(msg); // got to free msg

    // what if both server and client first send? -> no problem, we can even send simultaneously!
    char buf[1024];
    const char* username =  (args["--username"] ? args["--username"].asString().c_str() : "anon");
    sprintf(buf, "%s connected successfully", username);
    SendMessage(
        connection_fd,
        buf
    );
    close(connection_fd);

    return 0;
}
