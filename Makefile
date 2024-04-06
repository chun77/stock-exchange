CXX = g++
CXXFLAGS = -std=c++11 -Wall
LDFLAGS = -lpqxx
LMAINFLAGS = -lpqxx -ltinyxml2 -pthread
all: mockclient main client

mockclient: mockclient.cpp
	$(CXX) $(CXXFLAGS) -o mockclient mockclient.cpp

main: main.cpp server.cpp xmlSeqParser.cpp xmlSeqGenerator.cpp dbController.cpp
	$(CXX) $(CXXFLAGS) -o main main.cpp server.cpp xmlSeqParser.cpp xmlSeqGenerator.cpp dbController.cpp $(LMAINFLAGS)

client: client.cpp
	$(CXX) $(CXXFLAGS) -o client client.cpp

clean:
	rm -f main mockclient *.o