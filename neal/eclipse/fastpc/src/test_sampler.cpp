/*
 * test.cc
 *
 *  Created on: May 24, 2009
 *      Author: neal
 */

#include <iostream>

#include "sampler.h"

int test_sampler() {
	const int n = 100000;
	int exponents[n];
	int count[n+1];

	for (int i = 0;  i < n;  ++i)  {
		exponents[i] = 0;
		count[i] = 0;
	}
	count[n] = 0;

	Sampler* s = Sampler::create(10, n, exponents);

	for (int i = 0;  i < 20*n;  ++i) {
		int j = s->sample();
		assert(j >= -1 && j < n);
		count[j+1] += 1;
		s->decrement_exponent(i % (n - 1000));
	}
	int cum = 0;
	int max_i = 1;
	for (int i = 0;  i <= n;  ++i) {
		if (i > 0) cum += count[i];
		if (i > 0 && count[i] > count[max_i])  max_i = i;
		if (i < 100 || i % 10000 == 0 || i >= n - 100)
			std::cout << i-1 << " " << count[i] << " " << double(cum)/(i+1) << std::endl;
	}
	std::cout << "max: " << max_i-1 << " " << count[max_i] << std::endl;

	return 0;
}
