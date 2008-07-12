
include Makefile.included

LIB = ./lib
all: lib fastpc test

fastpc: fastpc.o $(LIB)/sampler.o $(LIB)/timer.o Makefile
	$(CXX) $(CXXFLAGS) -o fastpc fastpc.o $(LIB)/sampler.o $(LIB)/timer.o

fastpc: $(LIB)/timer.h $(LIB)/my_vector.h $(LIB)/sampler.h

.PHONY: test 
test:
	cd test && $(MAKE)

.PHONY: lib
lib:
	cd lib && $(MAKE)


clean: 
	rm -rf fastpc *.o
	cd test && $(MAKE) clean
	cd lib && $(MAKE) clean
