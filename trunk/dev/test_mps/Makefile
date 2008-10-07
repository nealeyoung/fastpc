
all:	glpsol_filter

glpsol_filter:	glpsol_filter.o
	$(CXX) glpsol_filter.o -o glpsol_filter

glpsol_filter.o: glpsol_filter.cc
	$(CXX) -c glpsol_filter.cc

clean:	
	rm -rf glpsol_filter *.o