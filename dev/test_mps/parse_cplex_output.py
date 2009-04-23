import sys
import re

def parse_cplex_output_file(file_name):
    try:
        my_file = open(file_name)
        total_string = my_file.read()
    except:
        print file_name + ', NOT FOUND'
        return

    time_reg = re.compile(r'Solution time = .*')
    name_reg = re.compile(r"File type: Problem '.*'")

    time_array = time_reg.findall(total_string)
    name_array = name_reg.findall(total_string)
    
    for index, item in enumerate(time_array):
        filename = str(name_array[index][name_array[index].rfind('/')+1:-1])
        input_list = filename.split('_')
        input_list[-1] = 'cplex'
        filename = '_'.join(input_list)
        density = input_list[-2]
        cols = input_list[-3]
        rows = input_list[-4]
        non_zeros = int(int(rows)*int(cols)*float(density))
        time_list = time_array[index].split()
        time = time_list[3]
        iter = time_list[-2] + ' ' + time_list[-1]
        print filename + ',' + rows + ',' + cols + ',' + str(non_zeros) + ',' + density + ',' + time + ',' + iter

def main():
    args = sys.argv
    try:
        file_prefix = args[1]
    except:
        print 'Usage: python parse_output.py <file_prefix>'
        sys.exit(1)

    cplex_file_name = './output_cplex/' + file_prefix + '_output' + '_cplex'

    output_file_name = './output_cplex/' + file_prefix + '_run_stats.csv'
    sys.stdout = open(output_file_name, 'w')
    print "Filename,Rows,Columns,Nonzeros,Density,Time (s),Iterations"

    parse_cplex_output_file(cplex_file_name)
    
main()
