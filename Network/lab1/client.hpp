#pragma once

#include <fstream>
#include <numeric>
#include <algorithm>

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

#include "utils.hpp"

using namespace std;

/* Client
 - read all files in shared directory
 - send request to server and wait for response
 - upload file chunks to peers
 - download file chunks from peers
*/
class Client{
    /* file meta information
     - file size
     - number of chunks
     - bitmap of chunks
    */
    typedef struct{
        uint fileSize;
        ushort nChunks;
        bitset<NUM_CHUNK> chunkmap;
    }_FileMetaInfo;
    
    ushort uploadPort;
    struct sockaddr_in uploadAddr;
    vector<pair<string, uint>> serverFileList;
    thread uploadThread;
    
    static int serverSocket, uploadSocket;
    static string filePath;
    static unordered_map<string, _FileMetaInfo> fileChunks;
    static mutex chunkLock;

    // add file to file list, including <file name, file size, chunk information>
    void addFile(const char *fileName, const uint fileSize){
        ushort nChunks = countChunks(fileSize);
        string name = string(fileName);
        char overwrite[5];
        if(fileChunks.find(name) != fileChunks.end()){
            cout << "[Client] " << name << " already exist, do you want to overwrite it?" << endl;
            printf("Overwrite? [y/n]: ");
            fflush(stdout);
            scanf("%s", overwrite);
            if(overwrite[0] != 'y' && overwrite[0] != 'Y')
                return;
        }
        _FileMetaInfo fileMeta;
        fileMeta.fileSize = fileSize;
        fileMeta.nChunks = nChunks;
        for(int i = 0;i < nChunks;++i)
            fileMeta.chunkmap.set(i);
        fileChunks[name] = fileMeta;
    }
    // read all files from directory
    bool makeFileList(){
        DIR *dir = opendir(filePath.c_str());
        if(dir == nullptr){
            printf("[Client] invalid directory\n");
            return false;
        }
        struct dirent *dirEntry = nullptr;
        struct stat statbuf;
        while((dirEntry = readdir(dir))){
            if(dirEntry->d_name[0] == '.')
                continue;
            stat((filePath + string(dirEntry->d_name)).c_str(), &statbuf);
            if(S_ISDIR(statbuf.st_mode)){
                printf("[Client] files in folder '%s' will not be added\n", dirEntry->d_name);
                continue;
            }
            addFile(dirEntry->d_name, statbuf.st_size);
            #ifdef DEBUG_CLIENT
            cout << "+ " << dirEntry->d_name << " (size: " << statbuf.st_size << ") added" << endl;
            #endif
        }
        closedir(dir);
        return true;
    }
    static bool checkSuccess(const packet& request, const ushort reqType){
        return request.packetType == reqType;
    }
    bool registerRequest(){
        packet request;
        uint n = fileChunks.size();
        /* fill in information of request
         - type: file register request
         - port: port used for sharing
         - value: number of files
        */
        request.packetType = REQ_REG;
        request.port = htons(uploadPort);
        request.value = htonl(n);
        // send number of files
        send(serverSocket, &request, sizeof(request), 0);
        // send register request of each file
        for(const auto& fileMeta: fileChunks){
            /* pack up information
             - file name
             - file size
            */
            strncpy(request.content, fileMeta.first.c_str(), fileMeta.first.size());
            request.content[fileMeta.first.size()] = 0;
            request.contentLength = htons(fileMeta.first.size());
            request.value = htonl(fileChunks[fileMeta.first].fileSize);
            send(serverSocket, &request, sizeof(request), 0);
        }
        // waiting for response from server
        recv(serverSocket, &request, sizeof(packet), MSG_WAITALL);
        if(!checkSuccess(request, REQ_REG)){
            printf("[Client] register failed\n");
            return false;
        }
        printf("[Client] %u file(s) register succeed\n", n);
        return true;
    }
    void fileListRequest(){
        packet request;
        /* fill in information of request
         - type: list request
        */
        request.packetType = REQ_LST;
        send(serverSocket, &request, sizeof(request), 0);
        // number of files is included in this response packet from server
        recv(serverSocket, &request, sizeof(packet), MSG_WAITALL);
        if(!checkSuccess(request, REQ_LST)){
            printf("[Client] list request failed\n");
            return;
        }
        uint n = ntohl(request.value);
        serverFileList.clear();
        // retrieving all files
        for(int i = 0;i < n;++i){
            recv(serverSocket, &request, sizeof(packet), MSG_WAITALL);
            if(!checkSuccess(request, REQ_LST)){
                printf("[Client] error in list\n");
                continue;
            }
            serverFileList.push_back(make_pair(string(request.content), ntohl(request.value)));
        }
        serverFileList.shrink_to_fit();
    }
    void outputFileList(){
        for(const pair<string, uint>& serverFile: serverFileList)
            cout << " + " << serverFile.first << " - size: " << serverFile.second << endl;
    }
    vector<LocInfo> fileLocationRequest(const string fileName){
        packet request;
        uint n = 0;
        /* fill in information of request
         - type: location request
         - content: file to be queried
        */
        request.packetType = REQ_LOC;
        strncpy(request.content, fileName.c_str(), fileName.size());
        request.content[fileName.size()] = 0;
        request.contentLength = htons(fileName.size());
        // send location request for 'fileName'
        send(serverSocket, &request, sizeof(request), 0);
        // number of peers holding this file is included
        recv(serverSocket, &request, sizeof(packet), MSG_WAITALL);
        if(!checkSuccess(request, REQ_LOC)){
            printf("[Client] location request failed\n");
            return {};
        }
        n = ntohl(request.value);
        if(n == 0){
            printf("[Client] no such a file\n");
            return {};
        }
        vector<LocInfo> fileLocation;
        // receive and extract location information
        for(int i = 0;i < n;++i){
            /* pack up information
             - peer ip
             - port
             - number of chunks
             - bitmap of chunks
            */
            recv(serverSocket, &request, sizeof(packet), MSG_WAITALL);
            fileLocation.push_back(LocInfo());
            fileLocation[i].ip = request.ip;
            fileLocation[i].port = ntohs(request.port);
            fileLocation[i].nChunks = ntohs(request.value);
            fileLocation[i].chunkmap = bitset<NUM_CHUNK>(request.content);
        }
        return fileLocation;
    }
    void outputFileLocation(const vector<LocInfo>& locations){
        struct in_addr addrInfo;
        bool full;
        for(const LocInfo& loc: locations){
            addrInfo.s_addr = loc.ip;
            full = loc.chunkmap.count() == loc.nChunks;
            cout << " + " << inet_ntoa(addrInfo) << ":" << loc.port << (full?" (integrated)":" (partial)") << endl;
        }
    }
    static bool chunkRegisterRequest(const ipaddr peer, const string fileName, const ushort chunk){
        packet request;
        /* fill in information of request
         - type: chunk register request
         - ip: peer's ip address
         - content: file name that is going to be registered
         - value: chunk number
        */
        request.packetType = REQ_CHK;
        request.ip = peer;              // denote which peer has received this chunk
        strncpy(request.content, fileName.c_str(), fileName.size());
        request.content[fileName.size()] = 0;
        request.contentLength = htons(fileName.size());
        request.value = htons(chunk);
        send(serverSocket, &request, sizeof(request), 0);
        recv(serverSocket, &request, sizeof(packet), MSG_WAITALL);
        if(!checkSuccess(request, REQ_CHK)){
            cout << "[Client] " << fileName << ":(" << chunk << ") register failed" << endl;
            return false;
        }
        return true;
    }
    bool leaveRequest(){
        packet request;
        /* fill in information of request
         - type: leaving request
        */
        request.packetType = REQ_LVE;
        send(serverSocket, &request, sizeof(request), 0);
        recv(serverSocket, &request, sizeof(packet), MSG_WAITALL);
        if(!checkSuccess(request, REQ_LVE)){
            printf("[Client] leave request failed\n");
            return false;
        }
        return true;
    }
    static void uploadChunks(const ipaddr peer, const uint peerSocket, const string fileName){
        packet request;
        datapack fileBuf;
        fstream fileIn(filePath + fileName, ios::in | ios::binary);
        ushort nChunks = fileChunks[fileName].nChunks, chunk;
        bool flag;
        while(true){
            // waiting for chunk request
            recv(peerSocket, &request, sizeof(request), MSG_WAITALL);
            if(!checkSuccess(request, REQ_DWN))
                printf("[Client:upload] request error\n");
            chunk = ntohs(request.value);       // chunk that is requested by peer
            if(chunk >= NUM_CHUNK)
                break;
            fileIn.seekg(chunk * CHUNK_SIZE, ios::beg);

            #ifdef DEBUG_CLIENT
            cout << "[Client:upload] uploading " << fileName << ":(" << chunk+1 << ")" << endl;
            #endif
            
            flag = true;
            for(int i = 0;flag && i < NUM_SEGMENT;++i){
                // read from file and pack all information up
                memset(fileBuf.content, 0, sizeof(fileBuf.content));
                fileIn.read(fileBuf.content, SEGMENT_SIZE);
                fileBuf.contentLength = fileIn.gcount();
                fileBuf.seqNum = i;
                if(fileBuf.contentLength == 0){
                    // end of transmission
                    fileBuf.seqNum = NUM_SEGMENT;
                    flag = false;
                }

                #ifdef DEBUG_CLIENT
                cout << " + segment " << fileBuf.seqNum << " ( " << fileBuf.contentLength << " ) uploaded" << endl;
                #endif

                send(peerSocket, &fileBuf, sizeof(fileBuf), 0);
            }
            // waiting for response
            // if value in response equals to the required chunk number
            // send chunk register request of this chunk to server
            recv(peerSocket, &request, sizeof(request), MSG_WAITALL);
            if(ntohs(request.value) == chunk)
                chunkRegisterRequest(peer, fileName, chunk);

            #ifdef DEBUG_CLIENT
            cout << "[Client:upload] " << fileName << ":(" << chunk+1 << ") succeed" << endl;
            #endif
        }
        fileIn.close();
        close(peerSocket);
    }
    static void uploadListen(){
        int peerSocket;
        uint sinSize = sizeof(struct sockaddr);
        packet request;
        struct sockaddr_in peerAddr;
        thread peerThread;
        while(true){
            // waiting for peer connection
            listen(uploadSocket, MAX_CONNECTION);
            if((peerSocket = accept(uploadSocket, (struct sockaddr *)&peerAddr, &sinSize)) < 0){
                printf("[Client:upload] connection failed\n");
                continue;
            }

            #ifdef DEBUG_CLIENT
            cout << "[Client:upload] peer connected" << endl;
            #endif

            // this packet will include the file that is wanted
            recv(peerSocket, &request, sizeof(packet), MSG_WAITALL);
            
            #ifdef DEBUG_CLIENT
            cout << "[Client:upload] uploading " << request.content << endl;
            #endif

            // setup a thread for peer transmission
            peerThread = thread(uploadChunks, peerAddr.sin_addr.s_addr, peerSocket, string(request.content));
            peerThread.detach();
        }
    }
    static vector<ushort> sortByRare(vector<LocInfo> fileLocations){
        ushort nChunks = fileLocations[0].nChunks;
        ushort chunkcount[nChunks];
        vector<ushort> chunkseq(nChunks, 0);
        iota(chunkseq.begin(), chunkseq.end(), 0);
        memset(chunkcount, 0, sizeof(chunkcount));
        for(const LocInfo& location: fileLocations){
            for(ushort i = 0;i < nChunks;++i){
                if(!location.chunkmap.test(i))
                    continue;
                ++chunkcount[i];
            }
        }
        sort(chunkseq.begin(), chunkseq.end(), [&chunkcount](const ushort& a, const ushort& b){
            return chunkcount[a] < chunkcount[b];
        });
        return chunkseq;
    }
    static void downloadChunks(const int peerSocket, const ipaddr peerip, const string fileName, const vector<ushort> chunkseq, const bitset<NUM_CHUNK> chunkmap){
        ushort nChunks = fileChunks[fileName].nChunks;
        struct in_addr peerAddr;
        peerAddr.s_addr = peerip;
        packet request;
        request.packetType = REQ_DWN;
        strncpy(request.content, fileName.c_str(), fileName.size());
        request.content[fileName.size()] = 0;
        request.contentLength = htons(fileName.size());
        send(peerSocket, &request, sizeof(request), 0);
        fstream fileOut(filePath + fileName, ios::out | ios::in | ios::ate | ios::binary);
        datapack fileBuf;
        for(ushort i: chunkseq){
            // if current peer does not have this chunk, continue
            if(!chunkmap.test(i))
                continue;
            // lock chunk i, means chunk i is going to be transmitted
            chunkLock.lock();
            if(fileChunks[fileName].chunkmap.test(i)){
                chunkLock.unlock();
                continue;
            }
            else
                fileChunks[fileName].chunkmap.set(i);
            chunkLock.unlock();
            
            #ifdef DEBUG_CLIENT
            cout << "[Client:download] requesting " << fileName << ":(" << i+1 << ")" << endl;
            #endif

            // send request to peer for chunk i
            request.value = htons(i);
            send(peerSocket, &request, sizeof(request), 0);
            fileOut.seekp(i * CHUNK_SIZE, ios::beg);
            for(int j = 0;j < NUM_SEGMENT;++j){
                // waiting for data from peer
                memset(fileBuf.content, 0, sizeof(fileBuf.content));
                recv(peerSocket, &fileBuf, sizeof(fileBuf), MSG_WAITALL);
                
                #ifdef DEBUG_CLIENT
                cout << " + segment " << fileBuf.seqNum << " ( " << fileBuf.contentLength << " ) downloaded" << endl;
                #endif
                
                // end of transmission
                if(fileBuf.contentLength == 0)
                    break;
                if(fileBuf.contentLength <= SEGMENT_SIZE){
                    // write to the file
                    fileOut.write(fileBuf.content, fileBuf.contentLength);
                    fileOut.flush();
                }
            }
            // transmission finished, tell the peer to register chunk i
            send(peerSocket, &request, sizeof(request), 0);
            cout << "[Client:download] " << fileName << ":(" << i+1 << ") finished ( from " << inet_ntoa(peerAddr) << " )" << endl;
        }
        fileOut.close();
        // tell the peer to stop transmission by setting the chunk number to some value out of range
        request.value = htons(NUM_CHUNK);
        send(peerSocket, &request, sizeof(request), 0);
        close(peerSocket);
    }
    void downloadFile(const string fileName, const bool rare){
        bool exist = false;
        _FileMetaInfo fileMeta;
        int peerSocket;
        uint nChunks = 0, n = 0;
        thread peerThreads[MAX_CONNECTION];
        struct sockaddr_in peerAddr;

        #ifndef DEBUG_CLIENT
        if(fileChunks.find(fileName) != fileChunks.end()){
            cout << "[Client:download] file already exist" << endl;
            return;
        }
        #endif

        // check if the file exists
        for(const pair<string, uint>& serverFile: serverFileList){
            if(serverFile.first == fileName){
                nChunks = countChunks(serverFile.second);
                fileMeta.fileSize = serverFile.second;
                fileMeta.nChunks = nChunks;
                fileMeta.chunkmap = bitset<NUM_CHUNK>();
                exist = true;
                break;
            }
        }
        if(!exist){
            cout << "[Client:download] no such a file" << endl;
            return;
        }
        /* get location information
         1. peer ip
         2. port used for sharing
         3. which chunk the peer is holding
         */
        vector<LocInfo> fileLocations = fileLocationRequest(fileName);
        if(fileLocations.empty())
            return;
        vector<ushort> chunkseq(nChunks, 0);
        fstream fileOut(filePath + fileName, ios::out | ios::binary);
        fileOut.close();
        fileChunks[fileName] = fileMeta;
        if(rare && nChunks > 1 && fileLocations.size() > 1)
            chunkseq = sortByRare(fileLocations);
        else
            iota(chunkseq.begin(), chunkseq.end(), 0);
        
        #ifdef DEBUG_CLIENT
        showVector(chunkseq);
        #endif
        
        for(const LocInfo& location: fileLocations){
            // setup connection to peer
            peerAddr.sin_family = AF_INET;
            peerAddr.sin_addr.s_addr = location.ip;
            peerAddr.sin_port = htons(location.port);
            if((peerSocket = socket(PF_INET, SOCK_STREAM, 0)) < 0){
                printf("[Client:download] socket creation failed\n");
                return;
            }
            if(connect(peerSocket, (struct sockaddr *)&peerAddr, sizeof(struct sockaddr)) < 0){
                printf("[Client:download] connection failed\n");
                close(peerSocket);
                return;
            }
            
            #ifdef DEBUG_CLIENT
            cout << "[Client:download] peer connected " << inet_ntoa(peerAddr.sin_addr) << ":" << location.port << endl;
            #endif

            // setup a thread to download from peer
            peerThreads[n++] = thread(downloadChunks, peerSocket, location.ip, fileName, chunkseq, location.chunkmap);
        }
        // wait for thread to finish transmission
        for(int i = 0;i < n;++i)
            if(peerThreads[i].joinable())
                peerThreads[i].join();
        exist = true;
        // check integrity
        for(int i = 0;i < nChunks;++i){
            if(!fileChunks[fileName].chunkmap.test(i)){
                cout << "[Client:download] " << fileName << ":(" << i << ") missing" << endl;
                exist = false;
            }
        }
        // report how many peers involved in this transmission
        if(exist)
            cout << "[Client:download] " << fileName << " downloaded from " << n << " peer(s)" << endl;
    }
    void help(){
        cout << "reg              - register all files in directory: " << filePath << endl;
        cout << "ls               - list all files that are being shared on the Internet" << endl;
        cout << "loc  <file name> - list all peer locations that are sharing the file" << endl;
        cout << "get  <file name> - download file from all sharing peers" << endl;
        cout << "getr <file name> - download file from all sharing peers (rarest first)" << endl;
        cout << "exit             - stop P2P sharing and exit" << endl;
    }
public:
    Client(const char *serverIp, const char *filePath, const ushort uploadPort){
        Client::filePath = string(filePath);
        if(Client::filePath[Client::filePath.size() - 1] != '/')
            Client::filePath.append("/");
        this->uploadPort = uploadPort;
        // setup connection to server
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr(serverIp);
        serverAddr.sin_port = htons(SERVER_PORT);
        if((Client::serverSocket = socket(PF_INET, SOCK_STREAM, 0)) < 0){
            printf("[Client] server socket creation failed\n");
            exit(0);
        }
        if(connect(Client::serverSocket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) < 0){
            printf("[Client] server connect failed\n");
            close(Client::serverSocket);
            exit(0);
        }
        // tell server which port will be used for sharing
        send(serverSocket, &uploadPort, sizeof(uploadPort), 0);
        // setup sharing socket
        uploadAddr.sin_family = AF_INET;
        uploadAddr.sin_addr.s_addr = INADDR_ANY;
        uploadAddr.sin_port = htons(uploadPort);
        if((uploadSocket = socket(PF_INET, SOCK_STREAM, 0)) < 0){
            printf("[Client:upload] socket creation failed\n");
            close(Client::serverSocket);
            exit(0);
        }
        if(bind(uploadSocket, (struct sockaddr *)&uploadAddr, sizeof(struct sockaddr)) < 0){
            printf("[Client:upload] socket binding failed\n");
            close(Client::serverSocket);
            close(Client::uploadSocket);
            exit(0);
        }
    }
    void run(){
        // start sharing
        uploadThread = thread(uploadListen);
        uploadThread.detach();
        char rawInput[32];
        ushort retry;
        string command, fileName;
        help();
        cout << endl;
        while(true){
            printf("> ");
            fflush(stdout);
            scanf("%s", rawInput);
            command = string(rawInput);
            retry = 0;
            if(command == "reg"){
                if(!makeFileList()){
                    leaveRequest();
                    break;
                }
                while(retry < RETRY && !registerRequest())
                    ++retry;
                if(retry >= RETRY)
                    cout << "[Client] file registering failed" << endl;
            }
            else if(command == "ls"){
                fileListRequest();
                outputFileList();
            }
            else if(command == "loc"){
                scanf("%s", rawInput);
                auto locations = fileLocationRequest(string(rawInput));
                outputFileLocation(locations);
            }
            else if(command == "get" || command == "getr"){
                scanf("%s", rawInput);
                downloadFile(string(rawInput), command == "getr");
            }
            else if(command == "exit"){
                leaveRequest();
                break;
            }
            else if(command == "help")
                help();
            else
                cout << "[Client] please use 'help' to see how to use" << endl;
        }
        close(serverSocket);
    }
};

string Client::filePath;
mutex Client::chunkLock;
int Client::serverSocket, Client::uploadSocket;
unordered_map<string, Client::_FileMetaInfo> Client::fileChunks;