include ../Makefile.included

LIB = ../lib

LIBS = timer.o sampler.o sampler.h my_vector.h main_include.h
TESTS = time_per_op test_sampler test_my_vector test_timer
OBJS = $(patsubst %,%.o,$(TESTS))

tests: $(TESTS)

$(TESTS) :  timer.o Makefile ../Makefile.included
$(OBJS) :  main_include.h Makefile ../Makefile.included
$(TESTS) : % : %.o

time_per_op: 
	$(CXX) $(CXXFLAGS) -o time_per_op time_per_op.o timer.o

test_sampler: sampler.o 
	$(CXX) $(CXXFLAGS) -o test_sampler test_sampler.o sampler.o timer.o

test_my_vector:
	$(CXX) $(CXXFLAGS) -o test_my_vector test_my_vector.o timer.o

test_timer: timer.o 
	$(CXX) $(CXXFLAGS) -o test_timer test_timer.o timer.o

test_my_vector.o : my_vector.h 
test_sampler.o: sampler.h my_vector.h 

$(LIBS): % : $(LIB)/%
	ln -sf $< $@

clean:
	rm -rf $(TESTS) *.o
