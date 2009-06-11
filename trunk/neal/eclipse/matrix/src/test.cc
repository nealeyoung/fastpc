/*
 * test.cc
 *
 *  Created on: May 24, 2009
 *      Author: neal
 */

#include <iostream>

#include "matrix.h"

int main() {
	Matrix::Entry entries[4];

	entries[0].set(0,0,2);
	entries[1].set(0,1,1);
	entries[2].set(1,0,1);
	entries[3].set(1,1,2);

	Matrix M(entries, 4);

	M.dump();

	M.remove_col(1);

	M.dump();

	return 0;
}
