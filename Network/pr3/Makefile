SERVER_SRC=server.cpp
CLIENT_SRC=client.cpp
CXXFLAGS=-std=c++11

server: $(SERVER_SRC)
	$(CXX) -o $@ $^

client: $(CLIENT_SRC)
	$(CXX) -o $@ $^

all: server client

clean:
	rm -f server client
	rm -f out.txt
