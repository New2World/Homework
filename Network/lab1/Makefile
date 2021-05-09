CPP		:= g++
CPPFLAG	:= -std=c++11 -O2 -Wno-unused-result
LNKLIB	:= -lpthread

all: client server

client: client.cpp
	@ $(CPP) $(CPPFLAG) client.cpp $(LNKLIB) -o client

server: server.cpp
	@ $(CPP) $(CPPFLAG) server.cpp $(LNKLIB) -o server

clean:
    ifneq ($(wildcard server),)
		@ rm server
    endif
    ifneq ($(wildcard client),)
		@ rm client
    endif