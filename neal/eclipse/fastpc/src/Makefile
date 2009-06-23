#For profiling
#CXXFLAGS = -DNDEBUG -g -pg

#For debugging
#CXXFLAGS = -g -Wall -Wextra -pedantic 

#For speed
CXXFLAGS = -O3

all: main

main: main.o matrix.o sampler.o get_time.o solver.o
	$(CXX) $(CXXFLAGS) -o fastpc main.o matrix.o get_time.o sampler.o solver.o
	rm *.o

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp

matrix.o: matrix.cpp
	$(CXX) $(CXXFLAGS) -c matrix.cpp

sampler.o: sampler.cpp
	$(CXX) $(CXXFLAGS) -c sampler.cpp

get_time.o: get_time.cpp
	$(CXX) $(CXXFLAGS) -c get_time.cpp

solver.o: solver.cpp
	$(CXX) $(CXXFLAGS) -c solver.cpp

clean: 
	rm -f fastpc *.o
