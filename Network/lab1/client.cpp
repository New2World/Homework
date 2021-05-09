#include "client.hpp"
#include "utils.hpp"

int main(int argc, char **argv){
    // arg check
    if(argc < 4){
        perror("Client: please provide server IP, the directory you wanna share and the port this client is going to use for sharing");
        return 0;
    }
    // start client
    Client client = Client(argv[1], argv[2], atoi(argv[3]));
    client.run();
    return 0;
}