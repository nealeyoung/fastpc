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
	typedef long long unsigned int Bigint;

	// Create a Sampler containing the n integers in initial_exponents[0..n-1].
	// The integer initial_exponents[i] is associated with id i.
	// (The array initial_exponents is used only for initialization, not for storage.)
	static Sampler* create				(int *initial_exponents, int n);

	// Increment the integer (thought of as an exponent) associated with id i.
	virtual void 	increment_exponent	(unsigned int id)				= 0;

	// Return a random id, or -1, where the probability of selecting any id
	// is proportional to 2^(integer associated with that id).
	virtual int 	sample				()								= 0;

	// Return an upper bound mantissa * 2^exponent on the sum of the weights.
	// (The upper bound is guaranteed to be at most twice the actual sum.)
	// In sample(), above, the probability of returning an id equals
	// 2^(integer associated with that id) / (this upper bound).
	virtual Bigint	total_weight_mantissa()						const	= 0;
	virtual int		total_weight_exponent()						const	= 0;

	// Remove an integer from the set.
	virtual void	remove				(unsigned int id)				= 0;

	// For debugging...
	virtual void	dump				()						const	= 0;

	virtual 		~Sampler			() 								{};
};


#endif /* SAMPLER_H_ */
