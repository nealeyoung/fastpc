/*
 * test.cc
 *
 *  Created on: May 24, 2009
 *      Author: neal
 */

#include <iostream>

#include "sampler.h"

int main() {
	int exponents[3] = {1,4,5};
	int count[4] = {0, 0, 0, 0};

	Sampler* s = Sampler::create(exponents, 3);

	s->dump();
	s->increment_exponent(0);
	s->dump();
	s->increment_exponent(0);
	s->dump();
	s->increment_exponent(0);
	s->dump();
	s->increment_exponent(0);
	s->dump();

	for (int i = 0;  i < 7000;  ++i) {
		int j = s->sample();
		//std::cout << j << std::endl;
		count[j+1] += 1;
		for (int j = 0;  j < 2;  ++j)
			s->increment_exponent(j);
		s->dump();
	}
	for (int i = 0;  i < 4;  ++i) {
		std::cout << i-1 << " " << count[i] << std::endl;
	}

	return 0;
}
