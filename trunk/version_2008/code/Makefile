
include Makefile.included

LIB = ./lib
all: lib fastpc test

fastpc:  fastpc.o $(LIB)/sampler.o $(LIB)/get_time.o Makefile
	$(CXX) $(CXXFLAGS)  -o  fastpc fastpc.o $(LIB)/sampler.o $(LIB)/get_time.o

fastpc: $(LIB)/my_vector.h $(LIB)/sampler.h fastpc.h

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
