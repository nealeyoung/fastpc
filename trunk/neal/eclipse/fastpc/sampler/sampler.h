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
	static Sampler* create(int *initial_exponents, int n);
	virtual void increment_exponent(unsigned int id) = 0;
	virtual int sample() = 0;
	virtual void remove(unsigned int id) = 0;
	virtual void dump() const = 0;
	virtual ~Sampler() {};
};

#endif /* SAMPLER_H_ */
