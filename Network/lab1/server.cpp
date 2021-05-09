#include "server.hpp"
#include "utils.hpp"

int main(int argc, char **argv){
    // start server
    Server server = Server();
    server.run();
    return 0;
}