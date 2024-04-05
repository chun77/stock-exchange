CXX = g++
CXXFLAGS = -std=c++11 -Wall
LDFLAGS = -lpqxx

dbController: dbController.cpp
	$(CXX) $(CXXFLAGS) -o dbController dbController.cpp $(LDFLAGS)

clean:
	rm -f dbController dbController.o