/*
 * solver.h
 *
 *  Created on: Jun 11, 2009
 *      Author: neal
 */

#ifndef SOLVER_H_
#define SOLVER_H_

class Solver {
public:
	typedef double Scalar;

	// Max { |x| : Mx <= 1 }
	// is UNBOUNDED (primal feasible, dual infeasible)
	// if there is an all-zero column.  Otherwise
	// it is BOUNDED (primal and dual feasible).

	static Solver* create(Scalar epsilon);
	virtual void add_entry(int row, int col, Scalar value) = 0;

	virtual bool solve() = 0; // return true if BOUNDED, false if UNBOUNDED

	virtual Scalar value_of_row_variable(int row) = 0;
	virtual Scalar value_of_col_variable(int col) = 0;

	virtual ~Solver() {};
};

#endif /* SOLVER_H_ */
