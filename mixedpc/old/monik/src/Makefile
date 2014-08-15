#For profiling
#CXXFLAGS = -DNDEBUG -g -pg

#For debugging and asserts
#CXXFLAGS = -g -Wall -Wextra

#For speed
CXXFLAGS = -DNDEBUG -O3

all: mxpc

mxpc: mixedpc.cpp matrix.cpp get_time.cpp solver.cpp solver.h matrix.h Makefile
	$(CXX) $(CXXFLAGS) -o mxpc mixedpc.cpp matrix.cpp get_time.cpp solver.cpp

clean: 
	rm -f mxpc *.o

test: mxpc
	mxpc 0.01 inputs/test_input0
	diff inputs/test_input0_out inputs/test_input0_0.01_out
