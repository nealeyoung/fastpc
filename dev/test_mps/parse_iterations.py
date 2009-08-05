import sys

cplex_run_stats = set()
itn_eps_stats = []
epsilon = 0.01

def parse_cplex_iterations(file_name, parse_time):
    global itn_eps_stats
    global epsilon
    global cplex_run_stats
    try:
        my_file = open(file_name)
    except:
        print file_name + ', NOT FOUND'
        return
    if parse_time:
        print "Filename,Time (s),Iterations,Method,"
    else:
        print "Filename,Iterations,Primal Objective,Dual Objective,Primal Eps, Dual Eps, Method,"
    method = ""
    method_found = False
    itn_data = []
    barrier_itn = False
    barrier_iterations = 0
    infeasible = False

    for line in my_file:
        if line.startswith("File type: Problem"): # New run

            input_name = line[line.rfind('/')+1:line.rfind(' ')-1]
            input_list = input_name.split('_')
            input_list[-1] = ''
            filename = '_'.join(input_list)
            density = input_list[-2]
            cols = input_list[-3]
            rows = input_list[-4]

            method_found = False
            infeasible = False
            method = ""
            itn_data = []
            barrier_itn = False

        if line.startswith("Presolve - Unbounded or infeasible"):
            infeasible = True
#            print infeasible

        if barrier_itn:
            if line.startswith("Barrier time ="):
                barrier_iterations = int(itn_array[0])
                barrier_itn = False
            else:
#                print line
                itn_array = line.split()
                itn_obj = [int(itn_array[0]), float(itn_array[1]), float(itn_array[2])]
#                print itn_obj
                itn_data.append(itn_obj)
            continue

        if (not parse_time or not method_found) and line.startswith("Iteration:"):
            itn_array = line.split()
            #print itn_array
            if not parse_time and not method == 'Barrier' and not itn_array[2] == 'Scaled':
                itn_obj = [int(itn_array[1]), float(itn_array[-1]), -1]
                #print itn_obj
                itn_data.append(itn_obj)
            if not method_found:
                if itn_array[2] == 'Objective':
                    method = "Primal"
                else:
                    method = "Dual"
                #print itn_array, method
                method_found = True

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

        if line.startswith("Primal simplex - Optimal:") or line.startswith("Dual simplex - Optimal"):
            final_obj = float(line.split()[-1])
            #print final_obj

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
            found_itn_for_eps = False
            for item in itn_data:
                if parse_time:
                    print input_name + "," + str(item[0]) + "," + str(item[1]) + "," + method
                else:
                    pr = item[1]
                    du = item[2]
                    p_eps = abs(final_obj - pr)/final_obj
                    d_eps = abs(du - final_obj)/final_obj
                    min_eps = p_eps
                    min_obj = pr
                    if p_eps > d_eps:
                        min_eps = d_eps
                        min_obj = du
                    if min_eps > epsilon:
                        found_itn_for_eps = False
                    elif not found_itn_for_eps:
                        found_itn_for_eps = True
                        total_iterations = final_iterations
                        if method == 'Barrier':
                            total_iterations = barrier_iterations
                        itn_eps_stats_data = input_name + "," + str(item[0]) + "," + str(min_obj) + "," + str(final_obj) + "," + str(min_eps) + "," + str(total_iterations) + "," + method
                        itn_eps_stats.append(itn_eps_stats_data)
                    print input_name + "," + str(item[0]) + "," + str(pr) + "," + str(du) + "," + str(p_eps) + "," + str(d_eps)  + "," + method

#            print infeasible
            if not infeasible:
                cplex_run_stats_data = filename + method + "," + str(rows) + "," + str(cols) + ","  + '---' + "," + str(density) + "," + str(final_time) + "," + str(final_iterations) + ","
            else:
                cplex_run_stats_data = filename + method + "," + str(rows) + "," + str(cols) + ","  + '---' + "," + str(density) + "," + 'INFEASIBLE OR UNBOUNDED' + "," + 'NOT KNOWN' + ","
#            print cplex_run_stats_data
            cplex_run_stats.add(cplex_run_stats_data)

            method = ""
            method_found = False
            itn_data = []
            barrier_itn = False
            barrier_iterations = 0

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

    cplex_file_name = './output_cplex/' + file_prefix + '_output_cplex'
    try:
        my_file = open(cplex_file_name)
    except:
        cplex_file_name = './output/' + file_prefix + '_output_cplex'
        print 'Warning: File not found in output_cplex directory. Using file in output directory'

    output_file_name = './output_cplex/' + file_prefix + '_itn_obj_stats.csv'
    sys.stdout = open(output_file_name, 'w')
    parse_cplex_iterations(cplex_file_name, False)
    sys.stdout.close()

    output_file_name = './output_cplex/' + file_prefix + '_itn_time_stats.csv'
    sys.stdout = open(output_file_name, 'w')
    parse_cplex_iterations(cplex_file_name, True)
    sys.stdout.close()

    output_file_name = './output_cplex/' + file_prefix + '_itn_eps_stats.csv'
    sys.stdout = open(output_file_name, 'w')
    global itn_eps_stats
    print "Filename, Iterations, Objective, Final Objective, Epsilon, Total Barrier Iterations, Method,"
    for row in itn_eps_stats:
        print row
    sys.stdout.close()

    output_file_name = './output_cplex/' + file_prefix + '_cplex_run_stats.csv'
    sys.stdout = open(output_file_name, 'w')
    global cplex_run_stats
    print "Filename, Rows, Columns, Nonzeros, Density, Time (s), Iterations,"
    for row in cplex_run_stats:
        print row

    sys.stdout.close()
    sys.stdout = sys.__stdout__

main()
