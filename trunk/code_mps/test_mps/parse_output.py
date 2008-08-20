import sys
import re

def parse_output_file(file_name, file_type):
    if file_type == 'fastpc':
        fastpc_run = True
        glpk_run = False
    else:
        fastpc_run = False
        glpk_run = True

    try:
        my_file = open(file_name)
        total_string = my_file.read()
    except:
        print file_name + ', NOT FOUND'
        return

    if fastpc_run:
        time_reg = re.compile(r' time = [0-9]*.*[0-9]*')
        ans_reg =  re.compile(r'primal = [0-9]*.[0-9]* dual = [0-9]*.[0-9]* ratio = [0-9]*.[0-9]*')
        eps_reg = re.compile(r'epsilon = 0.[0-9]*')
        input_reg = re.compile(r"ROWS: [0-9]* COLUMNS: [0-9]* NON ZERO's: [0-9]* DENSITY: [0-9]*.*[0-9]*")
        name_reg = re.compile(r"INPUT FILE: .[/a-zA-Z0-9._]*")

    elif glpk_run:
        time_reg = re.compile(r'[0-9]*.*[0-9]* secs')
        ans_reg = re.compile(r'obj =[ ]*[0-9]*.[0-9]*e[+-][0-9]*')
        input_reg = re.compile(r'[0-9a-zA-Z:_]* [0-9]* rows, [0-9]* columns, [0-9]* non-zeros') 
        name_reg = re.compile(r"[0-9a-zA-Z:_]* reading problem data from `[/0-9a-zA-Z._]*'")

    time_array = time_reg.findall(total_string)
    ans_array = ans_reg.findall(total_string)
    input_array = input_reg.findall(total_string)
    name_array = name_reg.findall(total_string)

    if fastpc_run:
        eps_array = eps_reg.findall(total_string)

    if glpk_run:
        for index, item in enumerate(time_array):

            print str(name_array[index][name_array[index].rfind('/')+1:-1])+',',
            input_list = input_array[index].split()
            rows = input_list[input_list.index("rows,")-1]
            cols =  input_list[input_list.index("columns,")-1]
            non_zeros = input_list[input_list.index("non-zeros")-1]
            density = float(non_zeros)/(float(rows)*float(cols))
            print str(rows)+',', str(cols)+',', str(non_zeros)+',', str(density)+',',
            time_list = time_array[index].split()
            print time_list[time_list.index("secs")-1]

    if fastpc_run:
        for index, item in enumerate(time_array):
            print str(name_array[index][name_array[index].rfind('/')+1:])+',',
            input_list = input_array[index].split()
            rows = input_list[input_list.index("ROWS:")+1]
            cols =  input_list[input_list.index("COLUMNS:")+1]
            non_zeros = input_list[input_list.index("ZERO's:")+1]
            density = float(non_zeros)/(float(rows)*float(cols))
            print str(rows)+',', str(cols)+',', str(non_zeros)+',', str(density)+',',
            time_list = time_array[index].split()
            print str(time_list[time_list.index("=")+1][:-1])+',',
            ans_list = ans_array[index].split()
            ans_list.reverse()
            ratio = ans_list[ans_list.index("=")-1]
            print str(ratio)+',',
            eps_list = eps_array[index].split()
            print eps_list[eps_list.index("=")+1]

def main():
    args = sys.argv
    try:
        file_prefix = args[1]
    except:
        print 'Usage: python parse_output.py <file_prefix>'
        sys.exit(1)

    fp_file_name = './output/' + file_prefix + '_output'
    glpk_file_name = fp_file_name + '_glpk'

    output_file_name = './output/' + file_prefix + '_run_stats'
    sys.stdout = open(output_file_name, 'w')
    print "Filename,Rows,Columns,Nonzeros,Density,Time(s),Ratio,Epsilon"

    parse_output_file(fp_file_name, 'fastpc')
    parse_output_file(glpk_file_name, 'glpk')
    
main()
