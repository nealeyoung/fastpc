/*
 * sampler.h
 *
 *  Created on: May 23, 2009
 *      Author: neal
 */

#ifndef SAMPLER_H_
#define SAMPLER_H_

class Sampler {
public:
	// Sampler(int *initial_exponents, int n) {};
	// Sampler() {};
	virtual void increment_exponent(unsigned int id) = 0;
	virtual int sample() = 0;
	virtual void remove(unsigned int id) = 0;
	virtual void dump() = 0;
	virtual ~Sampler() {};
};

Sampler* create_Sampler(int *initial_exponents, int n);

#endif /* SAMPLER_H_ */
