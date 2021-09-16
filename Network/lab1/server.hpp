#pragma once

#include "utils.hpp"

using namespace std;

/* data structure to keep file information
- file size
- number of clients that is holding this file
- number of chunks
*/
class FileMetaInfo{
    uint fileSize;
    uint nClients, nChunks;
    unordered_map<peerid, bitset<NUM_CHUNK>> clientList;

    bool checkChunk(const peerid client, const ushort chunk){
        return clientList[client].test(chunk);
    }
    bool setChunk(const peerid client, const ushort chunk){
        clientList[client].set(chunk);
        return checkChunk(client, chunk);
    }
public:
    FileMetaInfo(){
        nClients = 0;
    }
    FileMetaInfo(const uint fileSize){
        this->fileSize = fileSize;
        nChunks = countChunks(fileSize);
        nClients = 0;
    }
    uint getFileSize() const {
        return fileSize;
    }
    ushort getChunks() const {
        return nChunks;
    }
    uint getClients() const {
        return nClients;
    }
    vector<LocInfo> getClientInfo() const {
        vector<LocInfo> clients(nClients);
        int i = 0;
        for(const auto& fileInfo: clientList){
            clients[i].ip = (ipaddr)(fileInfo.first >> 32);
            clients[i].port = (ushort)(fileInfo.first & 0xffff);
            clients[i].nChunks = countChunks(fileSize);
            clients[i++].chunkmap = fileInfo.second;
        }
        return clients;
    }
    // register a chunk when chunk register request is received
    bool registerChunk(const peerid client, const ushort& chunk){
        if(chunk < 0 || chunk >= nChunks)
            return false;
        if(clientList.find(client) == clientList.end()){
            clientList[client] = bitset<NUM_CHUNK>();
            ++nClients;
        }

        #ifdef DEBUG_SERVER
        cout << "[Client] registering a chunk for " << "<ip>" << ":" << (ushort)(client & 0xffff) << endl;
        #endif

        return setChunk(client, chunk);
    }
    // remove a client when leave request is received
    bool removeClient(const peerid client){
        if(clientList.find(client) != clientList.end()){
            clientList[client].reset();
            clientList.erase(client);
            --nClients;
        }
        return true;
    }
};

/* data structure to keep client information
 - port used for sharing
 - file that client is sharing
*/
class ClientMetaInfo{
    ushort port;
    unordered_set<string> fileList;
public:
    ClientMetaInfo(){}
    ClientMetaInfo(const ushort port){
        this->port = port;
    }
    void setPort(const ushort port){
        this->port = port;
    }
    ushort getPort() const {
        return port;
    }
    const unordered_set<string>& getFileList() const {
        return fileList;
    }
    bool addFile(const string fileName){
        fileList.insert(fileName);
        return true;
    }
};

/* Server
Maintain file and client information
 - client ip -> client meta information
 - file name -> file meta information

Deal with requests
 - register
 - list
 - location
 - chunk register
 - leave
*/
class Server{
    int serverSocket;
    struct sockaddr_in serverAddr;

    #ifdef DEBUG_SERVER
    static uint nClients;
    #endif

    // mutex lock to protect racing data
    static mutex fileLock, clientLock, slotLock;
    static unordered_map<string, FileMetaInfo> fileList;
    static unordered_map<peerid, ClientMetaInfo> clientList;

    static void registerRequest(const peerid client, const int clientSocket, const packet regRequest){
        #ifdef DEBUG_SERVER
        cout << "[Server] Register Request" << endl;
        #endif

        packet request;
        // read port and number of files to be registered
        uint n = ntohl(regRequest.value), nChunks, fileSize;
        string fileName;
        bool check = true;
        clientLock.lock();
        fileLock.lock();

        #ifdef DEBUG_SERVER
        cout << "  + registering " << n << " files" << endl;
        #endif

        // waiting for file information to be registered
        for(int i = 0;i < n;++i){
            recv(clientSocket, &request, sizeof(request), MSG_WAITALL);
            fileName = string(request.content);
            fileSize = ntohl(request.value);
            nChunks = countChunks(fileSize);
            check = clientList[client].addFile(fileName);

            #ifdef DEBUG_SERVER
            cout << (i == n-1 ? "  └─ " : "  ├─ ") << fileName << " registered" << endl;
            #endif

            if(!check)
                break;
            // add file to file list
            if(fileList.find(fileName) == fileList.end())
                fileList[fileName] = FileMetaInfo(fileSize);
            // register all chunks of this file
            for(int i = 0;i < nChunks;++i){
                check &= fileList[fileName].registerChunk(client, i);
                if(!check)
                    break;
            }
        }
        fileLock.unlock();
        clientLock.unlock();
        // feedback
        if(check)
            request.packetType = REQ_REG;
        else
            request.packetType = 0;
        send(clientSocket, &request, sizeof(request), 0);
    }
    static void fileListRequest(const int clientSocket){
        #ifdef DEBUG_SERVER
        cout << "[Server] List Request" << endl;
        #endif

        uint n = 0;
        vector<pair<string, uint>> files;
        fileLock.lock();
        packet reply[fileList.size() + 1];
        // prepare packets for each file
        for(const pair<string, FileMetaInfo>& fileInfo: fileList){
            /* pack up information
             - file name
             - name length
             - file size
            */
            ++n;
            reply[n].packetType = REQ_LST;
            strncpy(reply[n].content, fileInfo.first.c_str(), fileInfo.first.size());
            reply[n].content[fileInfo.first.size()] = 0;
            reply[n].contentLength = htons(fileInfo.first.size());
            reply[n].value = htonl(fileInfo.second.getFileSize());
        }
        fileLock.unlock();
        // first packet tells the client number of files
        reply[0].packetType = REQ_LST;
        reply[0].value = htonl(n);
        for(int i = 0;i <= n;++i)
            send(clientSocket, &reply[i], sizeof(reply[i]), 0);
    }
    static void fileLocationRequest(const int clientSocket, packet locationRequest){
        #ifdef DEBUG_SERVER
        cout << "[Server] Location Request" << endl;
        #endif

        packet reply;
        reply.packetType = REQ_LOC;
        string chunkmap, reqFileName = string(locationRequest.content);
        // check if the file exists
        fileLock.lock();
        if(fileList.find(reqFileName) == fileList.end()){
            send(clientSocket, &reply, sizeof(reply), 0);
            fileLock.unlock();
            return;
        }
        // aggregate file location information
        FileMetaInfo fileInfo = fileList[reqFileName];
        fileLock.unlock();
        ushort nChunks = fileInfo.getChunks();
        uint n = fileInfo.getClients();
        reply.value = htonl(n);
        clientLock.lock();
        vector<LocInfo> locations = fileInfo.getClientInfo();
        clientLock.unlock();
        // tell the client how many peers is holding the file
        send(clientSocket, &reply, sizeof(reply), 0);
        reply.value = htons(nChunks);
        // send chunk information at each peer
        for(int i = 0;i < n;++i){
            /* pack up information
             - peer ip
             - port
             - number of chunks
             - bitmap of chunks
            */
            reply.ip = locations[i].ip;
            reply.port = htons(locations[i].port);
            chunkmap = locations[i].chunkmap.to_string();
            strncpy(reply.content, chunkmap.c_str(), chunkmap.size());
            reply.content[chunkmap.size()] = 0;
            reply.contentLength = htons(chunkmap.size());
            send(clientSocket, &reply, sizeof(reply), 0);
        }
    }
    static void chunkRegisterRequest(const int clientSocket, packet chunkRequest){
        #ifdef DEBUG_SERVER
        cout << "[Server] Chunk Register Request" << endl;
        #endif

        packet reply;
        peerid client = (peerid)chunkRequest.ip;
        string fileName = string(chunkRequest.content);
        ushort chunk = ntohs(chunkRequest.value);
        ushort port = ntohs(chunkRequest.port);
        bool check = true;
        client = (client << 32) | port;
        // add file to client
        clientLock.lock();
        clientList[client].addFile(fileName);
        clientLock.unlock();
        // register chunk to corresponding data structure
        fileLock.lock();
        check = fileList[fileName].registerChunk(client, chunk);
        fileLock.unlock();

        #ifdef DEBUG_SERVER
        cout << "  + " << fileName << ":(" << chunk+1 << ") registered" << endl;
        #endif

        // feedback
        if(check)
            reply.packetType = REQ_CHK;
        else
            reply.packetType = 0;
        send(clientSocket, &reply, sizeof(reply), 0);
    }
    static void leaveRequest(const peerid client, const int clientSocket){
        #ifdef DEBUG_SERVER
        cout << "[Server] Leave Request" << endl;
        #endif

        packet reply;
        unordered_set<string> clientFileList;
        bool check = true;
        clientLock.lock();
        fileLock.lock();
        clientFileList = clientList[client].getFileList();
        for(const string& fileName: clientFileList){
            // remove client from file meta information
            check = fileList[fileName].removeClient(client);
            if(!check)
                break;
            // if no client is holding this file, remove the file information from the list
            if(fileList[fileName].getClients() == 0)
                fileList.erase(fileName);
        }
        // remove client from client list
        if(check)
            clientList.erase(client);
        fileLock.unlock();
        clientLock.unlock();
        // feedback
        if(check)
            reply.packetType = REQ_LVE;
        else
            reply.packetType = 0;
        send(clientSocket, &reply, sizeof(reply), 0);
    }
    static void execute(const peerid client, const int clientSocket){
        packet request;
        bool flag = true;
        while(flag){
            recv(clientSocket, &request, sizeof(request), MSG_WAITALL);
            switch(request.packetType){
                case REQ_REG:
                registerRequest(client, clientSocket, request);
                break;
                case REQ_LST:
                fileListRequest(clientSocket);
                break;
                case REQ_LOC:
                fileLocationRequest(clientSocket, request);
                break;
                case REQ_CHK:
                chunkRegisterRequest(clientSocket, request);
                break;
                case REQ_LVE:
                leaveRequest(client, clientSocket);
                flag = false;

                #ifdef DEBUG_SERVER
                slotLock.lock();
                --nClients;
                slotLock.unlock();
                #endif
                
                break;
                default:
                printf("[Server] invalid request type (%hu)\n", (ushort)request.packetType);
                break;
            }
        }
        close(clientSocket);
    }
public:
    Server(){
        // setup server listening socket
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(SERVER_PORT);
        cout << "[Server] socket setup at port " << SERVER_PORT << endl;
        if((serverSocket = socket(PF_INET, SOCK_STREAM, 0)) < 0){
            printf("  - socket creation failed\n");
            exit(0);
        }
        if(bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) < 0){
            printf("  - socket binding failed\n");
            exit(0);
        }

        #ifdef DEBUG_SERVER
        nClients = 0;
        #endif
    }
    void run(){
        int clientSocket = 0;
        uint sinSize = sizeof(struct sockaddr);
        ushort port;
        peerid client;
        struct sockaddr_in clientAddr;
        thread clientThread;
        while(true){
            // waiting for client's connection
            listen(serverSocket, MAX_CONNECTION);
            if((clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &sinSize)) < 0){
                printf("[Server] client connection failed\n");
                continue;
            }
            client = (peerid)clientAddr.sin_addr.s_addr;
            cout << "[Server] " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << " connected" << endl;
            
            #ifdef DEBUG_SERVER
            slotLock.lock();
            ++nClients;
            cout << " + " << nClients << " online" << endl;
            slotLock.unlock();
            #endif

            // get port used by client for sharing
            recv(clientSocket, &port, sizeof(port), MSG_WAITALL);
            client = (client << 32) | port;
            // register client at server
            clientLock.lock();
            if(clientList.find(client) == clientList.end())
                clientList[client] = ClientMetaInfo(port);
            clientLock.unlock();
            // start a thread for each client
            clientThread = thread(this->execute, client, clientSocket);
            clientThread.detach();
        }
    }
};

mutex Server::fileLock, Server::clientLock;
unordered_map<string, FileMetaInfo> Server::fileList;
unordered_map<peerid, ClientMetaInfo> Server::clientList;

#ifdef DEBUG_SERVER
uint Server::nClients;
mutex Server::slotLock;
#endif