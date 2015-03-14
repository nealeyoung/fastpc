Beating Simplex Part Deux Paper
> I. Intro/Motivation
> > A. Summarize algorithm from (Koufogiannakis, Young 2007)
> > B. Goal: Extend implementation to cover non-0/1 matrices-- uniform increments, etc.
> > C. Goal #2: Perform more extensive testing, especially against CPLEX

> II. Implementation (Practical Issues)
> > A. The Data Structure
      1. Goal = sample/update from distro in O(1) time
> > > 2. Overview of theoretical data structure in papers
> > > 3. Precision issues-- 32/64 bits to store total weight in sampler structure
> > > > a. normalize exponents-- version2007 method and version2008 method
> > > > > (proportional weights, not absolute weights, are key for sampling-- just scales al

> > and probabilities stay the same)
> > > -- do while preprocessing (to ensure positive/negative exponents) and during upd


> -- handle overflow by dropping sampler item out of bucket structure

> -- compensation-- adjust calculation of random-pair to reflect correct relationship
> weight of samplers (algebra)
> > b. maintain approximate weights-- uniform weight within each bucket when updat

> weight in sampler
> > -- compensation-- longer sampling process; re-sample if necessary
> > > -- both at bucket level in sample() and sampler level in random\_pair()

> B. Main Algorithm
    1. Goal = accommodate non-uniform increments and arbitrary coeffs
> > 2. Overview of storage for M, MT, 4 samplers
> > > a. Choice of vector vs. list

> > 3. Preprocessing-- bound coeffs and sort rows/cols (rows of M and MT)

> III. Tests
> > A. Platform and Input Data (random with dif. densities, tomography


> B. Performance vs. predicted performance; constant factors

> C. Minor optimizations (potential or implemented)
    1. Sample/update tradeoff in bucket size
> > 2. Exact vs. approximate sorting-- when is it worth approx. sorting


> D. Performance vs. CPLEX

> E. Discussion of CPLEX, if any

# Comments #
Hm... the formatting didn't work out so well here.  I'm also going to add this outline to the repository.