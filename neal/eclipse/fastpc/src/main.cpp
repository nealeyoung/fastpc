/*
 * main.cpp
 *
 *  Created on: Jun 12, 2009
 *      Author: neal
 */
#include <iostream>
#include <cmath>

#include "solver.h"

int main() {
	double eps = 0.005;

	Solver* s = Solver::create(eps);

	int n_rows = 200;
	int n_cols = 300;
	double density = 0.1;

	for (int row = 0;  row < n_rows;  ++row)
		for (int col = 0;  col < n_cols;  ++col)
			if (std::rand() >= density * RAND_MAX)
				s->add_entry(row, col, std::pow(2.0, 7*(double(rand())/RAND_MAX-1)));

	if(s->solve()) {
		std::cout << "solved" << std::endl;
	} else {
		std::cout << "not solved" << std::endl;
	}
	double col_value = 0, row_value = 0;

	for (int row = 0;  row < n_rows;  ++row)  row_value += s->value_of_row_variable(row);
	for (int col = 0;  col < n_cols;  ++col)  col_value += s->value_of_col_variable(col);

	std::cout << "col value = " << col_value << ", row_value = " << row_value << std::endl;
	std::cout << "desired epsilon = " << eps << ", effective epsilon = " << row_value/col_value - 1.0 << std::endl;

	return 0;
}
