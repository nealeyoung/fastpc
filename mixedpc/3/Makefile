
# -ftrapv should cause abort with illegal instruction on signed integer overflow or underflow

CXXFLAGS = -std=c++11 -g -Wall -Wextra -ftrapv -DSAFEINT_ASSERT_ON_EXCEPTION

# CXXFLAGS = -DNDEBUG -std=c++11 -g -pg -ftrapv

CXX = g++ 

OBJDIR := .obj

TEST_CPP  := $(wildcard test_*.cpp)
TEST_STEM := $(TEST_CPP:%.cpp=%)

# filter to test_XXX such that XXX.cpp exists
XXX      := $(TEST_STEM:test_%=%)
WITH     := $(foreach xxx, $(XXX), $(if $(wildcard $(xxx).cpp), test_$(xxx)))
WITHOUT  := $(foreach xxx, $(XXX), $(if $(wildcard $(xxx).cpp), , test_$(xxx)))

all: $(TEST_STEM)

test_solver : $(OBJDIR)/approx_sum.o $(OBJDIR)/solver_matrix.o

$(WITH)    : test_% : $(OBJDIR)/test_%.o $(OBJDIR)/%.o
$(WITHOUT) : test_% : $(OBJDIR)/test_%.o


$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -MMD -o $@ $<
	@(cd $(OBJDIR); cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.P >> $*.d; \
          rm -f $*.P)

%: $(OBJDIR)/%.o 
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -rf $(OBJDIR)/*

-include $(OBJDIR)/*.d

