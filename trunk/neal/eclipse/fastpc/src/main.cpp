/*
 * main.cpp
 *
 *  Created on: Jun 12, 2009
 *      Author: neal
 */
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstdlib>
#include <cassert>

#include "solver.h"

#include <stdio.h>  // we use old FILE i/o because it seems to be faster than standard C++ i/o

double get_time();

int main(int argc, char *argv[]) {
	double 			eps			= 0;
	std::string		input_file	= "";
	//int			sort_ratio	= 1;
	std::string		usage = "Usage: fastpc <epsilon-factor> <filename> [sort-factor]";

	double			main_start_time = get_time();

	if (argc >= 3) {
		eps			= std::atof(argv[1]);
		input_file	= argv[2];
	}
	if (eps == 0  || argc > 4) {
		std::cerr << usage << std::endl;
		return -1;
	}
	if (argc == 4) {
		std::cerr << "error: sort-factor not implemented" << std::endl;
		return -1;
	}

	Solver* s = Solver::create(eps);
	{
		// we use old FILE i/o because it seems to be much faster than C++ i/o
		std::FILE* in_file = std::fopen(input_file.c_str(), "r");

		if (! in_file) {
			std::cerr << "Error opening " << input_file << std::endl;
			return -1;
		}

		std::cout << "INPUT FILE: " << input_file << std::endl;

		//read and parse 1st line of input (parameters)
		int R, C, r, c, total;
		double val;

		std::fscanf(in_file, "%d %d %d\n", &R, &C, &total);

		int non_zero_entry_count = 0;
	    while(true) {

	    	if (std::fscanf(in_file, "%d %d %lf\n", &r, &c, &val) != 3) break;
	    	if(non_zero_entry_count == total) {
	    		std::cout << "warning: input file claimed " << total << " non-zeros, but there are more (ignoring)" << std::endl;
	    		break;
	    	}
	    	assert(0 <= r && r < R && 0 <= c && c < C);
	    	if (val == 0)			continue;
			s->add_entry(r, c, val);
	    	++non_zero_entry_count;
	    }
	    fclose(in_file);

	    std::cout << "preprocessing_time0 = " << get_time() - main_start_time << " s" << std::endl;

	    s->done_adding_entries();

	    std::cout << "preprocessing_time1 = " << get_time() - main_start_time << " s" << std::endl;

	    assert(R == s->n_rows()  &&  C == s->n_cols()  &&  non_zero_entry_count == total);

	    std::cout << "ROWS: " <<  s->n_rows() << " COLUMNS: " << s->n_cols() << " NON-ZEROS: " << total
			<< " DENSITY: " << (double)total/((double)s->n_rows()*(double)s->n_cols())<< std::endl;
	}
	std::cout << "preprocessing_time = " << get_time() - main_start_time << " s" << std::endl;

	if(s->solve()) {
		std::cout << "solved" << std::endl;
	} else {
		std::cout << "not solved" << std::endl;
	}
	double col_value = 0, row_value = 0;

	for (int row = 0;  row < s->n_rows();  ++row)  row_value += s->value_of_row_variable(row);
	for (int col = 0;  col < s->n_cols();  ++col)  col_value += s->value_of_col_variable(col);

	std::cout << std::fixed << std::setprecision(3) << "primal: "
		  << col_value << " dual: " << row_value << " ratio: "
		  << col_value/row_value << std::endl;

	return 0;
}
