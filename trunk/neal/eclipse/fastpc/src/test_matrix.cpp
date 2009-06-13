/*
 * test.cc
 *
 *  Created on: May 24, 2009
 *      Author: neal
 */

#include <iostream>

#include "matrix.h"

int test_matrix() {
	Matrix M;

	M.add_entry(0,0,2,0);
	M.add_entry(0,1,1,0);
	M.add_entry(1,0,1,0);
	M.add_entry(1,1,2,0);
	M.done_adding_entries();

	M.dump();

	M.remove_col(1);

	M.dump();

	return 0;
}
