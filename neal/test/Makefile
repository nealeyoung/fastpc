include ../Makefile.included

LIB = ../lib

LIBS = get_time.o main_include.h
TESTS = time_per_op 
OBJS = $(patsubst %,%.o,$(TESTS))

tests: $(TESTS)

$(TESTS) :  get_time.o Makefile ../Makefile.included
$(OBJS) :  main_include.h Makefile ../Makefile.included
$(TESTS) : % : %.o

time_per_op: 
	$(CXX) $(CXXFLAGS) -o time_per_op time_per_op.o get_time.o

$(LIBS): % : $(LIB)/%
	ln -sf $< $@

clean:
	rm -rf $(TESTS) $(LIBS) *.o
