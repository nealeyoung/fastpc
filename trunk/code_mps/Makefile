
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
	cd tomog && $(MAKE)

glpsol_filter:	glpsol_filter.o
	$(CXX) glpsol_filter.o -o glpsol_filter

glpsol_filter.o: glpsol_filter.cc
	$(CXX) -c $(CXXFLAGS) glpsol_filter.cc


clean: 
	rm -rf fastpc glpsol_filter *.o
	cd test && $(MAKE) clean
	cd lib && $(MAKE) clean
	cd ../version_2007/code && $(MAKE) clean
	cd tomog && $(MAKE) clean
