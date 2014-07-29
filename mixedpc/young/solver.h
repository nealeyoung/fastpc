#ifndef _SOLVER_H_
#define _SOLVER_H_

#include <cstdlib>
#include <set>
#include <cassert>

#include "matrix.h"

class Solver {
 public:
  Solver(double epsilon) : eps(epsilon) {}
  ~Solver();

  void add_p_entry(int row, int col, double coeff);
  void add_c_entry(int row, int col, double coeff);
  void done_adding_entries();
  bool solve();
  void write_solution(std::string out_file_name);

  ///////////////////   stuff below is private /////////////////

 private:
  double eps, N;
  Matrix P, C;
                
  double* x;
  double* Px;
  double* Cx;

  bool  terminate;
  bool  infeasible;
  bool  unbounded;

  bool* constraints_dropped;
  bool  is_constraint_active(int i) { return ! constraints_dropped[i]; }

  size_t iterations;
  size_t phases;
        
  double local(int j);
  double global();
  double delta(int j);
  void   increment(int j, int round);

  void drop_constraint(int row, int j, double value);
  void update_constraints();
  void recompute_exact_values(int &j);

  double compute_Px(int i);
  double compute_Cx(int i);
  double compute_row(int i, Matrix &M);

  void scale_solution();
        
  int threshold_factor(int round);

  void check_constraints();
  bool check_local_for_phase(double local, int j, int factor);
        
  void quit_infeasible();
  void quit_unbounded();

  void print(double* arr, int len);
  void print(int* arr, int len);
  double abs_diff(double val1, double val2);
};

#endif // _SOLVER_H_
