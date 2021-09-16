#pragma once

#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <bitset>
#include <unordered_set>
#include <unordered_map>

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// server will wait for client's connection at this port
#define SERVER_PORT     12000

#define CHUNK_SIZE      1048576         // 1 MB
#define SEGMENT_SIZE    65536           // 64 KB

#define NUM_CHUNK       1024
#define NUM_SEGMENT     (CHUNK_SIZE / SEGMENT_SIZE)

#define RETRY           3
#define MAX_CONNECTION  32

#define REQ_REG         0x1
#define REQ_LST         0x2
#define REQ_LOC         0x3
#define REQ_CHK         0x4
#define REQ_LVE         0x5
#define REQ_DWN         0x6

// #define DEBUG_SERVER
// #define DEBUG_CLIENT

typedef unsigned long long peerid;
typedef uint ipaddr;

// file location
typedef struct{
    ipaddr ip;
    ushort port, nChunks;
    std::bitset<NUM_CHUNK> chunkmap;
} LocInfo;

// data structure for request/reply
typedef struct{
    ipaddr ip;
    uint value;
    ushort port, contentLength;
    char content[1025];
    char packetType;
} packet;

// data structure for file data transmission
typedef struct{
    uint contentLength, seqNum;
    char content[SEGMENT_SIZE];
} datapack;

// divide file into chunks
ushort countChunks(uint fileSize){
    ushort nChunks = fileSize / CHUNK_SIZE;
    if(nChunks * CHUNK_SIZE < fileSize)
        ++nChunks;
    return nChunks;
}

// for debugging: show information in request packet
#if defined(DEBUG_SERVER) || defined(DEBUG_CLIENT)

void showPacket(const packet& pkt){
    struct in_addr addr;
    addr.s_addr = pkt.ip;
    std::cout << "----------" << std::endl;
    std::cout << "Addr: " << inet_ntoa(addr) << ":" << ntohs(pkt.port) << std::endl;
    std::cout << "Packet Type: " << (uint)pkt.packetType << std::endl;
    std::cout << "Value: " << ntohl(pkt.value) << "  Length: " << ntohs(pkt.contentLength) << std::endl;
    std::cout << "Content: " << pkt.content << std::endl;
    std::cout << "----------" << std::endl;
}

template <typename T>
void showVector(const std::vector<T>& vec){
    for(const T& v: vec)
        std::cout << v << " ";
    std::cout << std::endl;
}
#endif