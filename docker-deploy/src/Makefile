CXX = g++
CXXFLAGS = -I//usr/lib/x86_64-linux-gnu/libtinyxml2.so.6 -std=c++11 -Wall
LMAINFLAGS = -lpqxx -ltinyxml2 -pthread
all: main 

main: main.cpp server.cpp xmlSeqParser.cpp xmlSeqGenerator.cpp dbController.cpp
	$(CXX) $(CXXFLAGS) -o main main.cpp server.cpp xmlSeqParser.cpp xmlSeqGenerator.cpp dbController.cpp $(LMAINFLAGS)

clean:
	rm -f main *.o