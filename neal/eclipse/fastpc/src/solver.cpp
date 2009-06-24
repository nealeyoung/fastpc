/*
 * solver.cpp
 *
 *  Created on: Jun 11, 2009
 *      Author: neal
 */

#include <cmath>
#include <iostream>

#include "solver.h"
#include "matrix.h"
#include "sampler.h"

#define inline

double get_time();

class _Solver : public Solver {
private:
	Scalar			_original_epsilon;
	Scalar			_epsilon;
	int				_k;

	Matrix			_M;
	Scalar*			_xc;  // col (packing) variables -- x in paper
	Scalar*			_xr;  // row (covering) variables -- x-hat in paper

	int exponent_of_value(Scalar value) {
	  static const Scalar log2 = std::log(2.0);
		// round value up to the next power of 1+epsilon = 2^(1/k), return exponent
		// 2^(i/k) >= value
		// i/k ln(2) >= ln(value)
		// i >= k ln(value) / ln(2)
	  return int(std::ceil(_k * std::log(value)/log2));
	}

	void random_pair(Sampler* pr, Sampler* pc, Sampler* pur, Sampler* puc,
			int* row, int* col) {
		int pr_M, pr_E,
			pc_M, pc_E,
			pur_M, pur_E,
			puc_M, puc_E;

		pr	->total_weight(&pr_M, 	&pr_E);
		pc	->total_weight(&pc_M, 	&pc_E);
		pur	->total_weight(&pur_M,	&pur_E);
		puc	->total_weight(&puc_M,	&puc_E);

		int p_RAND_MAX = int(RAND_MAX/(1.0 + std::ldexp((double(pr_M) * double(puc_M)) / (double(pur_M) * double(pc_M)),
								pr_E + puc_E - pur_E - pc_E)));
		// with probability |pur||pc| / ( |pur||pc| + |pr||puc| )    = 1/(1 + |pr||puc|/|pur||pc|)
		// choose row from pur and col from pc
		// otherwise
		// choose row from pr and col from puc

		do {
			if (std::rand() >= p_RAND_MAX) {
				*row = pur->sample();
				if (*row == -1) continue;
				*col = pc->sample();
				if (*col != -1) break;
			} else {
				*row = pr->sample();
				if (*row == -1) continue;
				*col = puc->sample();
				if (*col != -1) break;
			}
		} while (1);
	}

public:
	_Solver(Scalar epsilon) : _original_epsilon(epsilon), _xc(NULL), _xr(NULL) {
		assert(epsilon > 0);
		// round 1+epsilon down to next integer root of 2
		// 2^(1/k) <= 1+epsilon
		// 1/k ln(2) <= ln(1+epsilon)
		// k >= ln(2)/ln(1+epsilon)
		// _k = int(std::ceil(std::log(2.0)/std::log(1+epsilon)));
		// also, use smaller epsilon by 30%
		_k = int(std::ceil(std::log(2.0)/std::log(1+0.9*epsilon)));
		_epsilon = std::pow(2.0, 1.0/_k) - 1.0;
	}
	~_Solver() { if (_xr) delete[] _xr; if (_xc) delete[] _xc; }

	void add_entry(int row, int col, Scalar value) {
            assert(!_xr);
            _M.add_entry(row, col, value, exponent_of_value(value));
    }
	void done_adding_entries() {
		assert(!_xr);

		_M.done_adding_entries();

		int n_rows = _M.n_rows();
		int n_cols = _M.n_cols();

		assert(n_rows);
		assert(n_cols);

		_xr = new Scalar[n_rows];
		_xc = new Scalar[n_cols];

		assert(_xr && _xc);

		for (int row = 0;  row < n_rows;  ++row) _xr[row] = 0;
		for (int col = 0;  col < n_cols;  ++col) _xc[col] = 0;
	}

	int n_rows() { return _M.n_rows(); }
	int n_cols() { return _M.n_cols(); }

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

bool _Solver::solve() {
	double solve_start_time = get_time();

	assert(_xr);
	int nr = n_rows();
	int nc = n_cols();

	assert(nr);
	assert(nc);

	int yr[nr];		// y and y-hat from paper
	int yc[nc];

	assert(yr && yc);

	for (int row = 0;  row < nr;  ++row) {
		Matrix::Entry** entry = _M.max_entry_in_row(row);
		if (! entry) {
			// all-zero row -- trivial packing constraint
			// row (covering) variable occurs in no constraints
			// will delete it later
			yr[row] = 0;
		} else {
			yr[row] = (*entry)->_exponent;
		}
	}
	for (int col = 0;  col < nc;  ++col) {
		Matrix::Entry** entry = _M.max_entry_in_col(col);
		if (! entry) {
			// all-zero column -- covering constraint
			// -- packing variable is unconstrained (packing is UNBOUNDED)
			// -- covering constraint can't be satisfied (covering is INFEASIBLE)
			_xc[col] = 1;
			return false;
		} else {
			yc[col] = (*entry)->_exponent;
		}
	}

	Sampler*	pr		= Sampler::create(_k, nr);		// samplers from paper
	Sampler*	pc		= Sampler::create(_k, nc);
	Sampler*	pur		= Sampler::create(_k, nr, yr);
	Sampler*	puc		= Sampler::create(_k, nc, yc);

	for (int row = 0;  row < nr;  ++row) yr[row] = 0;
	for (int col = 0;  col < nc;  ++col) yc[col] = 0;

	// delete empty rows from row samplers
	for (int row = 0;  row < nr;  ++row)
		if (! _M.max_entry_in_row(row)) {
			// delete row
			pr->remove(row);
			pur->remove(row);
		}

	int N = int(std::ceil(2*std::log(nr*nc) / (_epsilon*_epsilon)));
	int iteration = 0, empty_iterations = 0;

	std::cout << "N = " << N << std::endl;

	double preprocessing_time = get_time() - solve_start_time;
	std::cout << "preprocessing_time2 = " << preprocessing_time << " s" << std::endl;

	double main_loop_start_time = get_time();

	do {
		bool empty_iteration = true;
		++iteration;

		int row, col;  // row is i',  col is j' (w.r.t. published alg)

		random_pair(pr, pc, pur, puc, &row, &col);
		assert(0 <= row && row < nr && 0 <= col && col < nc);

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
			empty_iteration = false;
			int row_prime = (*r)->_row;
			pr->increment_exponent(row_prime);
			pur->increment_exponent(row_prime);
			if (++yr[row_prime] >= N) break;
		}
		z_over_delta *= 1-_epsilon;
		for (Matrix::Entry** c = _M.first_entry_in_row(row, z_over_delta);
			  c;
			  c = _M.next_entry_in_row(row, c, z_over_delta)) {
			int col_prime = (*c)->_col;
			empty_iteration = false;
			pc->decrement_exponent(col_prime);
			puc->decrement_exponent(col_prime);

			if (++yc[col_prime] >= N) {
				// delete satisfied covering constraint col_prime

				std::list<int> rows_affected;  // collect rows whose max entries will be deleted
				for (Matrix::Entry** r = _M.first_entry_in_col(col_prime);
						r;
						r = _M.next_entry_in_col(col_prime, r)) {
					// r ranges over entries in col_prime
					if (*_M.max_entry_in_row((*r)->_row) == *r)
						rows_affected.push_back((*r)->_row);
				}
				// remove column col_prime from matrix and samplers
				_M.remove_col(col_prime);
				pc->remove(col_prime);
				puc->remove(col_prime);
				// update sampler pur, as max M_ij in row may have decreased
				for (std::list<int>::iterator it = rows_affected.begin();
						it != rows_affected.end();
						++it) {
					Matrix::Entry** max_in_row = _M.max_entry_in_row(*it);
					if (! max_in_row) {
						// No non-zeros in row in active columns.
						// => Row variable contributes to no active covering constraints.
						// => Remove that variable from samplers. (This is not done in published alg.)
						pr->remove(*it);
						pur->remove(*it);
					} else {
						// Recalculate exponent in pur.
						int row_exponent = yr[*it] + (*max_in_row)->_exponent;
						pur->decrease_exponent(*it, row_exponent);
					}
				}
			}
		}
		if (empty_iteration) ++empty_iterations;
	} while(! pc->empty());

	double main_loop_stop_time = get_time();

	// normalize variables
	_M.restore(); // undo removals
	// row is packing
	Scalar max_row_sum = 0, min_col_sum = 10*N, row_value = 0, col_value = 0;
	for (int row = 0;  row < nr;  ++row) {
		Scalar sum = 0;
		for (Matrix::Entry** c = _M.first_entry_in_row(row);  c;  c = _M.next_entry_in_row(row, c))
			sum += (*c)->_value * _xc[(*c)->_col];
		max_row_sum = std::max(sum, max_row_sum);
	}
	for (int col = 0;  col < nc;  ++col) {
		Scalar sum = 0;
		for (Matrix::Entry** r = _M.first_entry_in_col(col);  r;  r = _M.next_entry_in_col(col, r))
			sum += (*r)->_value * _xr[(*r)->_row];
		min_col_sum = std::min(sum, min_col_sum);
	}
	assert(max_row_sum > 0  &&  min_col_sum > 0);
	for (int row = 0;  row < nr;  ++row)  { _xr[row] /= min_col_sum;  row_value += _xr[row]; }
	for (int col = 0;  col < nc;  ++col)  { _xc[col] /= max_row_sum;  col_value += _xc[col]; }

	std::cout << " iterations = " << iteration << " primal = " << col_value << " dual = " << row_value
		<< " ratio = " << col_value/row_value << std::endl;
	std::cout << " empty iterations = " << empty_iterations << std::endl;

	std::cout << " eps = " << _epsilon << std::endl;
	std::cout << " epsilon = " << _original_epsilon << std::endl;

	std::cout << " sort ratio = " << 1 << std::endl;
	std::cout << " basic ops = " << -1 << std::endl;

	std::cout << " main_loop_time = " << main_loop_stop_time - main_loop_start_time << "s" << std::endl;
	std::cout << " post_processing_time = " << get_time() - main_loop_stop_time << "s" << std::endl;
	std::cout << " time = " << get_time() - solve_start_time << "s" << std::endl;

	return true;
}

Solver* Solver::create(Scalar epsilon) {
	return new _Solver(epsilon);
}

