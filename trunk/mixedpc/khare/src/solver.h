#ifndef SOLVER_H_
#define SOLVER_H_

#include <cstdlib>
#include <set>
#include <cassert>

#include "matrix.h"

class Fraction {
 private:
  double _num;
  double _den;
  double _num_mult;
  double _den_mult;	
 public:
  Fraction() {
    _num = 0;
    _num_mult = 1;
    _den = 1;
    _den_mult = 1;
  }

  ~Fraction() {
		
  }
	
  double num();
  double den();
  double value();
  double num_mult();
  double den_mult();
  void set_num(double n, double num_mult);
  void set_den(double d, double den_mult);
  void update_num(double change);
  void update_den(double change);
  void copy(Fraction f);
	
  friend std::ostream& operator<< (std::ostream &out, Fraction f) {
    out << f.num() * f.num_mult() << " / " << f.den() * f.den_mult() << " = " << f.value();
    return out;
  }
};

class Solver {
 public:
  long n_prows;
  long n_crows;
  long n_cols;

  // variables for time
  double input_reading_time;
  double input_total_time;
  double solving_time;
	
  Solver(double epsilon) {
    eps = epsilon;
  }
  void add_p_entry(int row, int col, double coeff);
  void add_c_entry(int row, int col, double coeff);
  void done_adding_entries();
  bool solve();
  void write_solution(std::string out_file_name);
	
  ~Solver();
	
 private:
  double eps, N;
  Matrix P, C;
		
  double* x;
  double* Px;
  double* Cx;
  double* delta;

  // to ignore the rows below a certain values while calculating local or global
  //	double P_cutoff;
  //	double C_cutoff;
	
  Fraction local;
  Fraction global;
  Fraction local_for_phase;

  double phase_increment;
  bool terminate;
  bool feasible;
  std::set<int> constraints_dropped;
  size_t iterations;
  size_t phases;
	
  void update_local(int j);
  void update_global();
  void update_delta();
  void update_constraints();
  void increment(int j, int round);
  void recompute_exact_values(int &j);
  void drop_constraint(int row, int j, double value);
  void scale_solution();
  bool is_constraint_active(int i);
	
  double compute_Px(int i);
  double compute_Cx(int i);
  double compute_row(int i, Matrix &M);

  // to verify calculations
  void check_constraints();
  bool check_local_for_phase(int j, int factor);
	
  void print(double* arr, int len) {
    for (int i = 0; i < len; ++i) {
      std::cout << arr[i] << " ";
    }
    std::cout << std::endl;
  }
	
  void print(int* arr, int len) {
    for (int i = 0; i < len; ++i) {
      std::cout << arr[i] << " ";
    }
    std::cout << std::endl;
  }
	
  void infeasible() {
    std::cout << " ---> Infeasible instance." << std::endl;
    terminate = true;
    feasible = false;
  }
	
  inline double abs_diff(double val1, double val2) {
    if (val1 > val2)
      return val1 - val2;
    return val2 - val1;
  }

  inline void next_variable(int &j) {
    j++;
    if (j == n_cols) {
      j = 0;
    }
    update_local(j);
  }
	
  inline int threshold_factor(int round) {
    int factor = 1;
    while (true) {
      if (round & 1) {
	break;
      } else {
	round /= 2;
	factor *= 2;
      }
    }
    return factor;
  }
};

#endif SOLVER_H_
