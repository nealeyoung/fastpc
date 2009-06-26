#For profiling
#CXXFLAGS = -DNDEBUG -g -pg

#For debugging
#CXXFLAGS = -g -Wall -Wextra -pedantic 
#CXXFLAGS = -g -Wall -Wextra

#For speed
CXXFLAGS = -DNDEBUG -O2

#CXXFLAGS = -DNDEBUG -g -pg

all: fastpc

fastpc: main.cpp matrix.cpp sampler.cpp get_time.cpp solver.cpp solver.h matrix.h sampler.h Makefile
	$(CXX) $(CXXFLAGS) -o fastpc main.cpp matrix.cpp get_time.cpp sampler.cpp solver.cpp

data: main.cpp matrix.cpp sampler.cpp get_time.cpp solver.cpp solver.h matrix.h sampler.h Makefile
	$(CXX) $(CXXFLAGS) -o fastpc main.cpp matrix.cpp get_time.cpp sampler.cpp solver.cpp -include sampler_output_flag.h

clean: 
	rm -f fastpc *.o
