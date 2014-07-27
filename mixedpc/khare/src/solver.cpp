
#include <cmath>
#include <iostream>
#include <fstream>

#include "solver.h"
#include "matrix.h"

double Fraction::num() {
  return _num;
}

double Fraction::den() {
  return _den;
}

double Fraction::value() {
  if (_num == 0)
    return 0;
  if (_den == 0)
    return -1;
  return (_num * _num_mult) / (_den * _den_mult);
}

double Fraction::num_mult() {
  return _num_mult;
}

double Fraction::den_mult() {
  return _den_mult;
}

void Fraction::set_num(double num, double num_mult) {
  _num = num;
  _num_mult = num_mult;
}

void Fraction::set_den(double den, double den_mult) {
  _den = den;
  _den_mult = den_mult;
}

void Fraction::update_num(double change) {
  _num += change / _num_mult;
}

void Fraction::update_den(double change) {
  _den += change / _den_mult;
}

void Fraction::copy(Fraction f) {
  _num = f._num;
  _den = f._den;
  _num_mult = f._num_mult;
  _den_mult = f._den_mult;
}

void Solver::add_p_entry(int row, int col, double value) {
  P.add_entry(row, col, value);
}

void Solver::add_c_entry(int row, int col, double value) {
  C.add_entry(row, col, value);
}

void Solver::done_adding_entries() {
  P.done_adding_entries();
  C.done_adding_entries();
	
  assert(!Px);
  assert(!Cx);
  assert(!x);
	
  n_prows = P.n_rows();
  n_crows = C.n_rows();
  n_cols = std::max(P.n_cols(), C.n_cols());
	
  Px = new double[n_prows];
  Cx = new double[n_crows];
  x = new double[n_cols];
  delta = new double[n_cols];
}

bool Solver::is_constraint_active(int i) {
  return constraints_dropped.find(i) == constraints_dropped.end();
}

void Solver::update_delta() {
  //	std::cout << "Updating delta for all variables." << std::endl;
  Matrix::Entry** p;
  Matrix::Entry** c;
  double d;
  for (int j = 0; j < n_cols; j++) {
    //		std::cout << "    j = " << j << std::endl;
    p = P.first_entry_in_col(j);
    c = C.first_entry_in_col(j);
    d = 0;
    if (p) {
      d = (*p)->_value;
    }
    if (c) {
      d = std::max(d, (*c)->_value);
    }
    //		std::cout << "    d = " << d << std::endl;
    if (d > 0)
      delta[j] = eps / d;
    else
      delta[j] = 0;
  }
}

void Solver::update_local(int j) {
  //	std::cout << "Updating local for variable " << j << std::endl;
  double value;
  double num = 0;
  double num_mult = 1;
  for (Matrix::Entry** e = P.first_entry_in_col(j); e; e = P.next_entry_in_col(j, e)){
    value = (*e)->_value * std::exp(Px[(*e)->_row]);
    num += value / num_mult;
  }
  //	std::cout << "Local numerator =  " << num << std::endl;

  double den = 0;
  double den_mult = 1;
  for (Matrix::Entry** e = C.first_entry_in_col(j); e; e = C.next_entry_in_col(j, e)){
    if (is_constraint_active((*e)->_row)) {
      //			std::cout << (*e)->_value << ", " << std::exp(-Cx[(*e)->_row]) << std::endl;
      value = (*e)->_value * std::exp(-Cx[(*e)->_row]);
      den += value / den_mult;
    }
  }
  //	std::cout << "Local denominator =  " << den << std::endl;
	
  local.set_num(num, num_mult);
  local.set_den(den, den_mult);
}

void Solver::update_global() {
  //        std::cout << "Updating global. Total iterations = " << iterations << " and phases = " << phases << std::endl;
  double num = 0;
  double num_mult = 1;
  for (int r = 0; r < n_prows; ++r) {
    num += std::exp(Px[r]) / num_mult;
  }
  double den = 0;
  double den_mult = 1;
  for (int r = 0; r < n_crows; ++r) {
    if (is_constraint_active(r)) {
      den += std::exp(-Cx[r]) / den_mult;
    }
  }
  global.set_num(num, num_mult);
  global.set_den(den, den_mult);
}

void Solver::update_constraints() {
  double value;
  
  for (int r = 0; r < n_prows; r++) {
    Px[r] = compute_Px(r);
  }

  for (int r = 0; r < n_crows; r++) {
    Cx[r] = compute_Cx(r);
  }
}

void Solver::drop_constraint(int row, int j, double value) {
  constraints_dropped.insert(row);
  // Delete row from C
  C.remove_row(row);

  // if the constraint was added back to check calculations, 
  // no other computations are required after dropping it again
  // this is a hack used only when asserts are ON
  if (value == 0)
    return;

  //	std::cout << "**********Dropping constraint " << row << " from C" << std::endl;

  Cx[row] = 0;
	
  // update the increments since the dropped constraint may change max coeff for some variables
  // this is important because the step size may become smaller
  update_delta();
	
  // If all C rows are dropped, terminate
  if (C.n_rows_curr() == 0) {
    terminate = true;
  }
}

bool Solver::check_local_for_phase(int j, int factor) {
  Matrix::Entry** max;
  //	int factor = threshold_factor(round);
  double threshold;
  int r;
  double row_increment;
	
  max = P.first_entry_in_col(j);
  double num = 0;
  if (max) {
    threshold = (*max)->_value / factor;

    for (Matrix::Entry** e = P.first_entry_in_col(j); e; e = P.next_entry_in_col(j, e, threshold)){
      r = (*e)->_row;
      row_increment = (*e)->_value * phase_increment;
      //			std::cout << " ---- row = " << r << ", coeff = " << (*e)->_value << std::endl;
      //			std::cout << " ---- row_increment = " << row_increment << ", num_increment = " << (*e)->_value * std::exp(Px[r] + row_increment) << std::endl;
      num += (*e)->_value * std::exp(Px[r] + row_increment);
    }
  }
	
  double den = 0;
  max = C.first_entry_in_col(j);
  if (max) {
    threshold = (*max)->_value / factor;
    for (Matrix::Entry** e = C.first_entry_in_col(j); e; e = C.next_entry_in_col(j, e, threshold)){
      r = (*e)->_row;
      if (is_constraint_active(r)) {
	row_increment = (*e)->_value * phase_increment;
	if (Cx[r] + row_increment < N) {
	  den += (*e)->_value * std::exp(-(Cx[r] + row_increment));
	}
      }
    }
  }
  //	std::cout << "Local for phase: " << local_for_phase.num() << "/ " << local_for_phase.den() << " = " << local_for_phase.value()	<< std::endl << std::flush;

  if (den != 0) {
    double val = num/den;
    //		std::cout << "Local for phase check: " << num << " / " << den << " = " << val << std::endl << std::flush;
    assert(abs_diff(val, local_for_phase.value()) < eps * val);
  } else {
    assert(local_for_phase.den() == 0);
  }
  return true;
}

void Solver::increment(int j, int round) {
  //	std::cout << "Increment: Round = " << round << ", variable = " << j << " and phase_increment = " << phase_increment << std::endl;
  // P_ij * exp(Px + d) = P_ij * exp(Px) * exp(d)
  // So, change in numerator of local = P_ij * exp(Px) * (exp(d) - 1)
  // Similarly, change in denominator of local = C_ij * exp(-Cx) * (exp(-d) - 1)
  // Since, d can be very small, we use expm1(d) to compute "exp(d) - 1"
	
  Matrix::Entry** max;
  int factor = threshold_factor(round);
  double threshold;
  int r;
  double row_increment;

  // local_for_phase should be set to local because the update is made based on phase_increment
  local_for_phase.copy(local);
	
  //	std::cout << "Local = " << local << std::endl;
	
  max = P.first_entry_in_col(j);
  if (max) {
    threshold = (*max)->_value / factor;
    for (Matrix::Entry** e = P.first_entry_in_col(j); e; e = P.next_entry_in_col(j, e, threshold)){
      r = (*e)->_row;
      row_increment = (*e)->_value * phase_increment;
      if (Px[r] + row_increment >= N) {
	terminate = true;
	return;
      }
      assert((*e)->_value * std::exp(Px[r]) * expm1(row_increment) > 0);
      //			std::cout << " ---- row = " << r << ", coeff = " << (*e)->_value << std::endl;
      //			std::cout << " ---- row_increment = " << row_increment << std::endl;
      local_for_phase.update_num((*e)->_value * std::exp(Px[r]) * expm1(row_increment));
    }
  }

  max = C.first_entry_in_col(j);
  if (max) {
    threshold = (*max)->_value / factor;
    for (Matrix::Entry** e = C.first_entry_in_col(j); e; e = C.next_entry_in_col(j, e, threshold)){
      r = (*e)->_row;
      row_increment = (*e)->_value * phase_increment;
      if (Cx[r] + row_increment >= N) {
	local_for_phase.update_den(-(*e)->_value * std::exp(-Cx[r]));
	drop_constraint(r, j, (*e)->_value);
      } else {
	assert((*e)->_value * std::exp(-Cx[r]) * expm1(-row_increment) < 0);
	local_for_phase.update_den((*e)->_value * std::exp(-Cx[r]) * expm1(-row_increment));
      }
    }
  }

  if (! C.first_entry_in_col(j)) {
    local_for_phase.set_den(0, 1);
  }
  assert(check_local_for_phase(j, factor));
}

double Solver::compute_Px(int i) {
  return compute_row(i, P);
}

double Solver::compute_Cx(int i) {
  return compute_row(i, C);
}

double Solver::compute_row(int i, Matrix &M) {
  double sum = 0;
  for (Matrix::Entry** e = M.first_entry_in_row(i); e; e = M.next_entry_in_row(i, e)){
    sum += (*e)->_value * x[(*e)->_col];
  }
  return sum;  
}

void Solver::recompute_exact_values(int &j) {
  //	std::cout << "Recompute exact: " << ", variable = " << j << " and phase_increment = " << phase_increment << std::endl;
  // exp(Px + d) = exp(Px) * exp(d) = exp(Px) * (1 + d), for small d
  // So, increase in numerator of global = exp(Px) * d
  // Similarly, exp(-Cx - d) = exp(-Cx) * exp(-d) = exp(-Cx) * (1 - d), for small d
  // So, decrease in denominator of global = exp(-Cx) * d
	
  int r;
  double row_increment;
  double g_incr;
  for (Matrix::Entry** e = P.first_entry_in_col(j); e; e = P.next_entry_in_col(j, e)){
    r = (*e)->_row;
    row_increment = (*e)->_value * phase_increment;

    g_incr = std::exp(Px[r]) * expm1(row_increment);
    global.update_num(g_incr);
    local.update_num((*e)->_value * g_incr);

    Px[r] += row_increment;
		
    assert(abs_diff(Px[r], compute_Px(r)) < eps);

    //		if (Px[r] > Px_max.value) {
    //			Px_max.set(r, Px[r]);
    //		}

    if (Px[r] >= N) {
      terminate = true;
      return;
    }
  }

  for (Matrix::Entry** e = C.first_entry_in_col(j); e; e = C.next_entry_in_col(j, e)){
    r = (*e)->_row;
    row_increment = (*e)->_value * phase_increment;

    g_incr = std::exp(-Cx[r]) * expm1(-row_increment);
    global.update_den(g_incr);
    local.update_den((*e)->_value * g_incr);
		
    Cx[r] += row_increment;
		
    assert(abs_diff(Cx[r], compute_Cx(r)) < eps);

    //		if (Cx[r] > Cx_max.value) {
    //			Cx_max.set(r, Cx[r]);
    //		}

    if (Cx[r] >= N) {
      drop_constraint(r, j, (*e)->_value);
    }
  }
}

void Solver::scale_solution() {
  //	std::cout << "Scale solution by " << N << std::endl;
  for (int j = 0; j < n_cols; j++) {
    x[j] = x[j] / N;
  }
}

bool Solver::solve() {
  //	P.dump();
  //	C.dump();

  int m = n_prows + n_crows;

  N = 2 * log(m) / eps;
  //	std::cout << "N = " << N << std::endl;
	
  double eps_plus_1 = eps + 1;
	
  // todo: use correct values for cutoffs
  //	P_cutoff = 0;
  //	C_cutoff = 0;
	
  size_t expected_itr = int(2 * m * log(m) / (eps*eps));
	
  //	std::cout << "Expected iterations = " << expected_itr << std::endl;
  size_t itr_print_step = expected_itr/1000;
  if (itr_print_step == 0) {
    itr_print_step = 100;
  }

  feasible = true;
  terminate = false;
  phases = 0;
  iterations = 0;
		
  int j = 0; //round-robin index
  long dead_count = 0;
  long dead_count_limit = (n_cols << 1) + 2;
  long dead_count_recompute_limit = n_cols - 2;
	
  int round;
  phase_increment = 0;

  update_global();
  update_delta();
  update_local(j);

  while (true) {
    if (terminate)
      break;

    //		std::cout << "Phase with variable " << j << std::endl;

    if (j == 0) {
      if (dead_count >= dead_count_recompute_limit) {
	//	std::cout << "Updating all constraints!!!! ----------- ---- ---- ---" << std::endl;
	update_constraints();
      }
      //		        std::cout << "dead count = " << dead_count << " and dead count limit = " << dead_count_limit << std::endl;
      update_global();// do this only once in every round robin cycle
    }

    local_for_phase.copy(local);

    phases++;
    //		std::cout << "Total phases = " << phases << std::endl;

    round = 1;
    phase_increment = 0;

    //		std::cout << "starting new phase " << std::endl;
		
    while (true) {
      // check for errors in each iteration
      // comment this when done testing
      // check_all_values(j);

      if (terminate) {
	break;
      }
			
      iterations++;
      //			if (iterations % itr_print_step == 0) {
      //				std::cout << "Total iterations = " << iterations << std::endl;
      //			}

      //			std::cout << "Local = " << local.value() << " for variable " << j << std::endl;
      //			std::cout << "Approximate local = " << local_for_phase.value() << " for variable " << j << std::endl;
      //			std::cout << "Global = " << global.value() << std::endl;

      if (local_for_phase.den() > 0 && delta[j] > 0 
	  && (((local_for_phase.num_mult() / global.num_mult()) * (global.den_mult() / local.den_mult())) 
	      * (local_for_phase.num() * global.den()) / (global.num() * local_for_phase.den()) <= eps_plus_1)) {
	//new phase of increments begins
	x[j] += delta[j];
	phase_increment += delta[j];
	increment(j, round);
	round++;
      } else {
	//				std::cout << "End of phase" << std::endl;
	if (dead_count > dead_count_limit) {
	  infeasible();
	  break;
	}
	if (phase_increment > 0) {
	  dead_count = 0;
	  recompute_exact_values(j);
	} else {
	  dead_count++;
	  next_variable(j);
	}
	break;
      }
    }
  }
	
  //	std::cout << "Actual iterations = " << iterations << std::endl;

  if (!feasible)
    return false;

  scale_solution();

  check_constraints();
		
  return true;
}

void Solver::check_constraints() {
  P.restore();
  double row_sum;
  double row_max = 1 + eps;
  //	std::cout << "Constraints in P: " << std::endl;
  for (int r = 0; r < n_prows; r++) {
    row_sum = compute_Px(r);
    assert(row_sum <= row_max);
    //		std::cout << "Value of constraint " << r << " is " << row_sum << std::endl;
  }
  C.restore();
  //	std::cout << "Constraints in C: " << std::endl;
  for (int r = 0; r < n_crows; r++) {
    row_sum = compute_Cx(r);
    //		std::cout << "Value of constraint " << r << " is " << row_sum << std::endl;
    assert(row_sum >= 1);
  }
}

void Solver::write_solution(std::string out_file_name) {
  double itr_per_phase;
  if (phases > 0) {
    itr_per_phase = (double) iterations / phases;
  }
  std::ofstream out_file(out_file_name.c_str());
  out_file << "Epsilon = " << eps << std::endl << std::endl;
	
  out_file << "Phases completed = " << phases << std::endl;
  out_file << "Iterations completed = " << iterations << std::endl;
  out_file << "Average iterations per phase = " << itr_per_phase << std::endl << std::endl;
	
  out_file << "Time for reading input = " << input_reading_time << " s" << std::endl;
  out_file << "Total time for reading and processing input = " << input_total_time << " s" << std::endl;
  out_file << "Time for solving (doesn't include input reading/preprocessing time) = " << solving_time << " s" << std::endl << std::endl;

  if (!feasible) {
    out_file << std::endl << "Instance is infeasible." << std::endl;
    return;
  }

  double objective = 0;
  for (int j = 0; j < n_cols; j++) {
    objective += x[j];
  }
  out_file << "Objective value = " << objective << std::endl;
  out_file << "Variable values: " << std::endl;
  for (int j = 0; j < n_cols; j++) {
    out_file << x[j] << " ";
  }
  out_file << std::endl;
  out_file.close();
}
