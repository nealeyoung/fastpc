include ../Makefile.included

LIB = ../lib

LIBS = get_time.o sampler.o sampler.h my_vector.h main_include.h
TESTS = time_per_op test_sampler test_my_vector test_get_time
OBJS = $(patsubst %,%.o,$(TESTS))

tests: $(TESTS)

$(TESTS) :  get_time.o Makefile ../Makefile.included
$(OBJS) :  main_include.h Makefile ../Makefile.included
$(TESTS) : % : %.o

time_per_op: 
	$(CXX) $(CXXFLAGS) -o time_per_op time_per_op.o get_time.o

test_sampler: sampler.o 
	$(CXX) $(CXXFLAGS) -o test_sampler test_sampler.o sampler.o get_time.o

test_my_vector:
	$(CXX) $(CXXFLAGS) -o test_my_vector test_my_vector.o get_time.o

test_get_time: get_time.o 
	$(CXX) $(CXXFLAGS) -o test_get_time test_get_time.o get_time.o

test_my_vector.o : my_vector.h 
test_sampler.o: sampler.h my_vector.h 

$(LIBS): % : $(LIB)/%
	ln -sf $< $@

clean:
	rm -rf $(TESTS) $(LIBS) *.o
