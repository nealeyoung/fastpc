
include Makefile.included

LIB = ./lib
all: lib fastpc test v07 tomog glpsol_filter

fastpc:  fastpc.o $(LIB)/sampler.o $(LIB)/get_time.o Makefile
	$(CXX) $(CXXFLAGS)  -o  fastpc fastpc.o $(LIB)/sampler.o $(LIB)/get_time.o

fastpc: $(LIB)/my_vector.h $(LIB)/sampler.h fastpc.h

.PHONY: test 
test:
	cd test && $(MAKE)

.PHONY: lib
lib:
	cd lib && $(MAKE)

.PHONY:	v07
v07:
	cd ../version_2007/code && $(MAKE)

.PHONY:	tomog
tomog:
	cd test_mps/tomog && $(MAKE)

.PHONY:	glpsol_filter
glpsol_filter:
	cd test_mps && $(MAKE)

clean: 
	cd test && $(MAKE) clean
	cd lib && $(MAKE) clean
	cd ../version_2007/code && $(MAKE) clean
	cd test_mps/tomog && $(MAKE) clean
	cd test_mps && $(MAKE) clean
