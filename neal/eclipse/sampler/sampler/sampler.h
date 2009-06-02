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
	// Create a Sampler containing the n integers in initial_exponents[0..n-1].
	// The integer initial_exponents[i] is associated with id i.
	// (The array initial_exponents is used only for initialization, not for storage.)
	// right now k is assumed > 0.
	static Sampler* create				(int k, int *initial_exponents, int n);

	// Increment the integer (thought of as an exponent) associated with id i.
	virtual void 	increment_exponent	(unsigned int id)				= 0;

	// Decrement the integer (thought of as an exponent) associated with id i.
	virtual void 	decrement_exponent	(unsigned int id)				= 0;

	// Return a random id, or -1, where the probability of selecting any id
	// is proportional to the id weight:  2^(integer associated with that id / k).
	virtual int 	sample				()								= 0;

	// Return an upper bound mantissa * 2^exponent on the sum of the weights.
	// In sample(), above, the probability of returning an id exactly equals
	// 2^(integer associated with that id / k) / (mantissa * 2^exponent)
	// The probability of returning -1 is the remaining probability.
	virtual void	total_weight		(int& mantissa, int& exponent)	= 0; // const

	// Remove an integer from the set.
	virtual void	remove				(unsigned int id)				= 0;

	// For debugging...
	virtual void	dump				()						const	= 0;

	virtual 		~Sampler			() 								{};
};


#endif /* SAMPLER_H_ */
