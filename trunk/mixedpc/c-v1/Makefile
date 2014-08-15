
CXXFLAGS = -g -Wall -Wextra

# CXXFLAGS = -DNDEBUG -g -pg

CXX = g++ -std=c++11

test_solve: test_solve.o solve.o matrix.o
	$(CXX) $(CXXFLAGS) test_solve.o solve.o matrix.o -o test_solve

test_solve.o: test_solve.cpp solve.h matrix.h
	$(CXX) $(CXXFLAGS) -c test_solve.cpp -o test_solve.o

matrix.o: matrix.cpp matrix.h
	$(CXX) $(CXXFLAGS) -c matrix.cpp -o matrix.o

solve.o: solve.cpp solve.h
	$(CXX) $(CXXFLAGS) -c solve.cpp -o solve.o

clean:
	rm -rf *.o
