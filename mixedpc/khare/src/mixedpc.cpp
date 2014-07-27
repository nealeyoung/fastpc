
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdlib>
#include <cassert>

#include "solver.h"

double get_time();

int main(int argc, char *argv[]) {
	double start_time = get_time();
	double eps = 0;
	std::string file_name = "";
	std::string out_file_name = "";
	std::string usage = "mxpc <epsilon> <in-file-name>";

	if (argc < 3) {
		std::cerr << usage << std::endl;
		return -1;
	}
	
	eps = std::atof(argv[1]);
	file_name = argv[2];
	
	out_file_name = file_name + "_" + argv[1] + "_out";
	
	Solver* S = new Solver(eps);
	
	std::ifstream in_file(file_name.c_str());
	if (in_file.fail()) {
		std::cerr << "Error opening " << file_name << std::endl;
		return -1;
	}
	
	std::cout << "mxpc starting..." << std::endl;
	std::cout << " ---> Epsilon = " << eps << std::endl;
	std::cout << " ---> Reading input from '" << file_name << "'" << std::endl;
	
	int P_ROWS, C_ROWS, COLS, TOTAL;
	int p_or_c, r, c;
	double value;
	char buffer[1024];
	
	in_file >> P_ROWS >> C_ROWS >> COLS >> TOTAL;

	int entry_count = 0;
	bool error = false;
	
	while(true) {
		char *p = buffer;
		in_file.getline(p, 1024);
		
		if (in_file.eof()) {break;}
		
		if (entry_count == TOTAL) {
			std::cout << "Warning: Claimed " << TOTAL << " entries but file contains more." << std::endl;
			break;
		}

		p_or_c = int(strtol(p, &p, 10));
		r = int(strtol(p, &p, 10));
		c = int(strtol(p, &p, 10));
		value = strtod(p, &p);
		
//		std::cout << p_or_c << " " << r << " " << c << " " << value << std::endl;

		if (value == 0) continue;

		entry_count++;

		assert(p_or_c == 1 || p_or_c == 2);
		assert(r >= 0 && c >= 0 && value >= 0);

		if (value > 0) {
			assert(c < COLS);
			if (p_or_c == 1) {
				assert(r < P_ROWS);
				S->add_p_entry(r, c, value);
			} else {
				assert(r < C_ROWS);
				S->add_c_entry(r, c, value);
			}
		}
	}
	
	if (error) {
		std::cout << "Fix errors with input and try again." << std::endl;
		return 1;
	}
	std::cout << " ---> Done reading input " << std::endl;
	S->input_reading_time = get_time() - start_time;

	S->done_adding_entries();
	
	std::cout << " ---> Done processing input " << std::endl;
	S->input_total_time = get_time() - start_time;

//	std::cout << " ---> #P_ROWS = " << P_ROWS << " #C_ROWS = " << C_ROWS << " #VARS = " << COLS << " #NON_ZEROS = " << entry_count << std::endl;
//	std::cout << " ---> #P_ROWS = " << S->n_prows << " #C_ROWS = " << S->n_crows << " #VARS = " << S->n_cols << " #NON_ZEROS = " << TOTAL << std::endl;
//	assert(P_ROWS == S->n_prows && C_ROWS == S->n_crows && entry_count == TOTAL);
	
	std::cout << " ---> Computing solution... " << std::endl;

	start_time = get_time();
	S->solve();
	S->solving_time = get_time() - start_time;

	std::cout << " ---> Writing solution to " << out_file_name << std::endl;
	S->write_solution(out_file_name);
	
	std::cout << " ---> DONE. " << std::endl << std::endl;
	
	return 0;
}