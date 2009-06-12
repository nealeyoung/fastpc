/*
 * solver.cpp
 *
 *  Created on: Jun 11, 2009
 *      Author: neal
 */

#include <cmath>

#include "solver.h"
#include "matrix.h"

class _Solver : public Solver {
private:
	Scalar			_epsilon;
	int				_k;

	Matrix			_M;
	Scalar*			_xc;  // col (packing) variables -- x in paper
	Scalar*			_xr;  // row (covering) variables -- x-hat in paper

	int exponent_of_value(Scalar value) {
		// round value up to the next power of 1+epsilon = 2^(1/k), return exponent
		// 2^(i/k) >= value
		// i/k ln(2) >= ln(value)
		// i >= k ln(value) / ln(2)
		return int(std::floor(_k * std::log(value)/std::log(2.0)));
	}

public:
	_Solver(Scalar epsilon) : _xc(NULL), _xr(NULL) {
		assert(epsilon > 0);
		// round 1+epsilon down to next integer root of 2
		// 2^(1/k) <= 1+epsilon
		// 1/k ln(2) <= ln(1+epsilon)
		// k >= ln(2)/ln(1+epsilon)
		_k = int(std::floor(std::log(2.0)/std::log(1+epsilon)));
		_epsilon = std::pow(2.0, 1.0/_k);
	}
	void add_entry(int row, int col, Scalar value) {
		_M.add_entry(row, col, value, exponent_of_value(value));
	}
	bool solve();
	Scalar value_of_row_variable(int row) {
		assert(_xr);
		assert(0 <= row  &&  row < _M.n_rows());
		return _xr[row];
	}
	Scalar value_of_col_variable(int col) {
		assert(_xc);
		assert(0 <= col  &&  col < _M.n_cols());
		return _xc[col];
	}
};

#include "sampler.h"

bool _Solver::solve() {
	_M.done_adding_entries();

	int n_rows = _M.n_rows();
	int n_cols = _M.n_cols();

	assert(n_rows);
	assert(n_cols);

	_xr = new Scalar(n_rows);
	_xc = new Scalar(n_cols);

	int row_exponents[n_rows];
	int col_exponents[n_cols];

	assert(_xr && _xc && row_exponents && col_exponents);

	for (int row = 0;  row < n_rows;  ++row) _xr[row] = 0;
	for (int col = 0;  col < n_cols;  ++col) _xc[col] = 0;

	for (int row = 0;  row < n_rows;  ++row) {
		Matrix::Entry** entry = _M.max_entry_in_row(row);
		if (! entry) {
			// all-zero row -- trivial packing constraint
			// row (covering) variable occurs in no constraints
			// will delete it later
			row_exponents[row] = 0;
		} else {
			row_exponents[row] = (*entry)->_exponent;
		}
	}
	for (int col = 0;  col < n_cols;  ++col) {
		Matrix::Entry** entry = _M.max_entry_in_col(col);
		if (! entry) {
			// all-zero column -- covering constraint
			// -- packing variable is unconstrained (packing is UNBOUNDED)
			// -- covering constraint can't be satisfied (covering is INFEASIBLE)
			_xc[col] = 1;
			return false;
		} else {
			col_exponents[col] = (*entry)->_exponent;
		}
	}

	Sampler*	pr		= Sampler::create(_k, n_rows);
	Sampler*	pc		= Sampler::create(_k, n_cols);
	Sampler*	pr_ur	= Sampler::create(_k, n_rows, row_exponents);
	Sampler*	pc_uc	= Sampler::create(_k, n_cols, col_exponents);

	for (int row = 0;  row < n_rows;  ++row)
		if (! _M.max_entry_in_row(row)) {
			// delete row
			pr->remove(row);
			pr_ur->remove(row);
		}

	int N = int(std::ceil(2*std::log(n_rows*n_cols) / (_epsilon*_epsilon)));

	do {
		int row, col;  // row is i',  col is j'

		random_pair(pr, pc, pr_ur, pc_uc, &row, &col);
		// TODO: write random_pair

		Matrix::Entry** row_max = _M.max_entry_in_row(row);
		Matrix::Entry** col_max = _M.max_entry_in_col(col);

		assert(row_max);
		assert(col_max);

		Scalar delta = 1.0/((*row_max)->_value + (*col_max)->_value);
		// (assumes _value's are not integers so division is not rounded)

		_xr[row] += delta;
		_xc[col] += delta;

		Scalar z_over_delta = (Scalar(std::rand()) / delta) / Scalar(RAND_MAX);

		for (Matrix::Entry** r = _M.first_entry_in_col(col, z_over_delta);
			  r;
			  r = _M.next_entry_in_col(col, r, z_over_delta)) {
			pr->increment_exponent((*r)->_row);
			pr_ur->increment_exponent((*r)->_row);
		}
		for (Matrix::Entry** c = _M.first_entry_in_row(row, z_over_delta);
			  c;
			  c = _M.next_entry_in_row(row, c, z_over_delta)) {
			pc->increment_exponent((*c)->_col);
			pc_uc->increment_exponent((*c)->_col);
			// TODO: delete col if pc exponent reaches N
			// and BREAK loop if all cols are gone.
			// (consider case when row becomes empty -> all cov constraints sat -> freeze row var)
		}
	} while(1);
	return true;
}
