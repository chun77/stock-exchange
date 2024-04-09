CXX = g++
CXXFLAGS = -std=c++11 -Wall
LMAINFLAGS = -lpqxx -ltinyxml2 -pthread -L/usr/lib/x86_64-linux-gnu/
all: main 

main: main.cpp server.cpp xmlSeqParser.cpp xmlSeqGenerator.cpp dbController.cpp
	$(CXX) $(CXXFLAGS) -o main main.cpp server.cpp xmlSeqParser.cpp xmlSeqGenerator.cpp dbController.cpp $(LMAINFLAGS)

clean:
	rm -f main *.o