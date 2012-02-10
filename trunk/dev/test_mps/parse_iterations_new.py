#!/usr/bin/env python

import sys
import re

cplex_run_stats = set()
itn_eps_stats = []
epsilon = 0.01
hybrid_data = []

input_params = {}

def load_input_params(file_prefix):
    param_file = open('parsed_input_params.txt')
    for line in param_file:
        arr = line.split('$')
        if arr[0].startswith(file_prefix+'#'):
            input_params[arr[0].split('#')[1]] = arr[1]

def parse_cplex_iterations(file_name, parse_time, out_file):
    global itn_eps_stats
    global epsilon
    global cplex_run_stats
    global hybrid_data

    try:
        my_file = open(file_name)
    except:
        out_file.write(file_name + ', NOT FOUND\n')
        return
    if parse_time:
        out_file.write("Filename,Time (s),Iterations,Method,\n")
    else:
        out_file.write("Filename,Iterations,Primal Objective,Dual Objective,Primal Eps, Dual Eps, Method,\n")
    method = ""
    method_found = False
    itn_data = []
    barrier_itn = False
    barrier_iterations = -1
    barrier_time = -1
    infeasible = False
    
    new_instance = False

    for line in my_file:
        if line.startswith("File type: Problem") or line.startswith("Problem"): # New run
            new_instance = True
            input_name = line[line.rfind('/')+1:line.rfind(' ')-1]
            input_list = input_name.split('_')
            input_list[-1] = ''
            filename = '_'.join(input_list)
            param_data = input_params[filename[:-1]].split('#')
#	    print param_data
            rows = int(param_data[0])
            cols = int(param_data[1])
            nonzeros = int(param_data[2])
            density = float(param_data[3])
            if nonzeros == 0:
                nonzeros = 12*int(rows)
            hybrid_eps = 1.0/nonzeros
            
            
            method_found = False
            infeasible = False
            method = ""
            itn_data = []
            barrier_itn = False
            
        if not new_instance:
            continue

        elif line.startswith("Presolve - Unbounded or infeasible"):
            infeasible = True
#            print infeasible

        if barrier_itn:
            if line.startswith("Barrier time ="):
                barrier_time = float((line.split())[-2])
                barrier_itn = False
                itn_data_len = len(itn_data)
                itn_obj_last = itn_data[itn_data_len -1]
                barrier_iterations = int(itn_array[0])
                if parse_time:
                    itn_obj_last[0] = barrier_time
                    itn_data[itn_data_len -1] = itn_obj_last
            else:
#                print line
                itn_array = line.split()
                if parse_time:
                    itn_obj = [-1, int(itn_array[0])]
                else:
                    itn_obj = [int(itn_array[0]), float(itn_array[1]), float(itn_array[2])]
#                print itn_obj
                itn_data.append(itn_obj)
            continue

        if line.startswith("Primal simplex - Optimal:") or line.startswith("Dual simplex - Optimal"):
            arr = line.split()
            final_obj = float(arr[-1])
            #print final_obj
            if not method_found:
                method = arr[0]
                method_found = True

        if (not parse_time or not method_found) and line.startswith("Iteration:"):
            itn_array = line.split()
            #print itn_array

            if itn_array[2] == 'Scaled':
                continue

            if not parse_time and method != 'Barrier':
                itn_obj = [int(itn_array[1]), float(itn_array[-1]), float(itn_array[-1])]
                itn_data.append(itn_obj)

        if not method_found and line.startswith(" Itn"):
            method = "Barrier"
            method_found = True
            barrier_itn = True

        if parse_time and line.startswith("Elapsed time"):
            itn_time_array = line.split()
            #print itn_time_array
            if itn_time_array[3] > 0:
                time_itn = [float(itn_time_array[3]), int(itn_time_array[-2].replace('(', ''))]
                #print time_itn
                itn_data.append(time_itn)

        if line.startswith("Solution time"): # End of run
            sol_array = line.split()
            #print sol_array
            final_time = sol_array[3]
            final_time_unit = sol_array[4]
            final_iterations = 0
            if len(sol_array) > 5:
                final_iterations = sol_array[-2]
            if final_iterations > 0:
                if not parse_time:
                    itn_data.append([final_iterations, final_obj, final_obj])
                else:
                    itn_data.append([final_time, final_iterations])
            #print final_time, final_time_unit, final_iterations
            if method == 'Barrier':
                final_iterations = str(int(final_iterations) + barrier_iterations)
            found_itn_for_eps = False
            found_itn_for_hybrid = False
            for item in itn_data:
                if parse_time:
                    out_file.write(filename + method + "," + str(item[0]) + "," + str(item[1]) + "," + method + '\n')
                else:
                    pr = item[1]
                    du = item[2]
                    if method == 'Primal':
                        du = -1
                    elif method == 'Dual':
                        pr = -1
                    if pr != -1:
                        p_eps = abs(final_obj - pr)/final_obj
                    else:
                        p_eps = -1
                    if du != -1:
                        d_eps = abs(du - final_obj)/final_obj
                    else:
                        d_eps = -1
                    min_eps = p_eps
                    min_obj = pr
                    if p_eps == -1 or (p_eps > d_eps and d_eps != -1):
                        min_eps = d_eps
                        min_obj = du
                    #Hybrid data
                    if min_eps > hybrid_eps or min_eps == -1:
                        found_itn_for_hybrid = False
                    elif not found_itn_for_hybrid:
                        found_itn_for_hybrid = True
                        hybrid_itr = item[0]
                        hybrid_data_stats = filename + method + ", " + str(hybrid_itr) + ", " + str(min_obj) + ", " + str(final_obj) + ", " + str(min_eps) + ", " + str(final_iterations) + ", " + str(hybrid_eps)  + ", " + method
                        hybrid_data.append(hybrid_data_stats)
                    
                    #Target epsilon data
                    if min_eps > epsilon or min_eps == -1:
                        found_itn_for_eps = False
                    elif not found_itn_for_eps:
                        found_itn_for_eps = True
                        total_iterations = final_iterations
                        if method == 'Barrier':
                            total_iterations = barrier_iterations
                        eps_itr = int(item[0])
                        itn_eps_stats_data = filename + method + ", " + str(eps_itr) + ", " + str(min_obj) + ", " + str(final_obj) + ", " + str(min_eps) + ", " + str(total_iterations) + ", " + str(epsilon) + ", " + method
                        itn_eps_stats.append(itn_eps_stats_data)

                    out_file.write(filename + method + ", " + str(item[0]) + ", " + str(pr) + ", " + str(du) + ", " + str(p_eps) + ", " + str(d_eps)  + ", " + method + '\n')

#            print infeasible
            if not parse_time and not infeasible:
                temp_iter = eps_itr
                if int(final_iterations) == 0:
                    temp_time = 0
                    hybrid_time = 0
                else:
                    temp_time = float(final_time)*temp_iter/float(final_iterations)
                    hybrid_time = float(final_time)*(temp_iter/float(final_iterations))
                if method == 'Barrier':
                    if barrier_iterations == 0:
                        temp_time = 0
                    else:
                        temp_time = barrier_time*(float(temp_iter)/barrier_iterations)
#                    temp_iter = barrier_iterations
                cplex_run_stats_data = filename + method + ", " + str(rows) + ", " + str(cols) + ", "  + str(nonzeros) + ", " + str(density) + ", " + str(temp_time) + ", " + str(temp_iter) + ", -1, -1, " + str(final_time) +  ", " + str(final_iterations) + ", " + str(hybrid_itr) + ", " + str(hybrid_time) + ", "
#                print cplex_run_stats_data
                cplex_run_stats.add(cplex_run_stats_data)

            method = ""
            method_found = False
            itn_data = []
            barrier_itn = False
            barrier_iterations = 0
            barrier_time = 0
            hybrid_eps = -1
            hybrid_itr = -1
            eps_itr = -1
            temp_iter = -1
            temp_time = -1

            new_instance = False

def main():
    args = sys.argv
    global epsilon
    try:
        file_prefix = args[1]
        if len(args) > 2:
            epsilon = float(args[2])
    except:
        print 'Usage: python parse_output.py <file_prefix> [eps]'
        sys.exit(1)

    load_input_params(file_prefix)

    cplex_file_name = './output_cplex/' + file_prefix + '_output_cplex'

    try:
        my_file = open(cplex_file_name)
    except:
        cplex_file_name = './output/' + file_prefix + '_output_cplex'
        print 'Warning: File not found in output_cplex directory. Using file in output directory'

    output_file_name = './output_cplex/' + file_prefix + '_itn_obj_stats.csv'
    out_file = open(output_file_name, 'w')
    parse_cplex_iterations(cplex_file_name, False, out_file)
    out_file.close()

    output_file_name = './output_cplex/' + file_prefix + '_itn_time_stats.csv'
    out_file = open(output_file_name, 'w')
    parse_cplex_iterations(cplex_file_name, True, out_file)
    out_file.close()

    output_file_name = './output_cplex/' + file_prefix + '_itn_eps_stats.csv'
    itn_eps_file = open(output_file_name, 'w')
    global itn_eps_stats
    itn_eps_file.write("Filename, Iterations, Objective, Final Objective, Epsilon, Total Barrier Iterations, Target Epsilon, Method,\n")
    for row in itn_eps_stats:
        itn_eps_file.write(row + '\n')
    itn_eps_file.close()

    output_file_name = './output_cplex/' + file_prefix + '_hybrid_stats.csv'
    hybrid_stats_file = open(output_file_name, 'w')
    global hybrid_data
    hybrid_stats_file.write("Filename, Iterations, Objective, Final Objective, Epsilon, Total Iterations, Hybird Epsilon, Method,\n")
    for row in hybrid_data:
        hybrid_stats_file.write(row + '\n')
    hybrid_stats_file.close()

    output_file_name = './output_cplex/' + file_prefix + '_cplex_run_stats.csv'
    run_stats_file = open(output_file_name, 'w')
    global cplex_run_stats
    run_stats_file.write("Filename,Rows,Columns,Nonzeros,Density,Time(s),Iterations,Ratio,Epsilon,TotalTime,TotalIter,HybridIter,HybridTime,\n")
    for row in cplex_run_stats:
        run_stats_file.write(row + '\n')

    run_stats_file.close()

main()
