import sys
import re

def parse_fastpc_output_file(file_name, v07):
    try:
        my_file = open(file_name)
        total_string = my_file.read()
    except:
        print file_name + ', NOT FOUND'
        return

    if not v07:
        sort_reg = re.compile(r'sort ratio = [0-9]*.[0-9]*')
    time_reg = re.compile(r' time = [0-9]*.*[0-9]*')
    ans_reg =  re.compile(r'primal = [0-9]*.[0-9]* dual = [0-9]*.[0-9]* ratio = [0-9]*.[0-9]*')
    eps_reg = re.compile(r'epsilon = 0.[0-9]*')
    input_reg = re.compile(r"ROWS: [0-9]* COLUMNS: [0-9]* NON-ZEROS: [0-9]* DENSITY: [0-9]*.*[0-9]*")
    name_reg = re.compile(r"INPUT FILE: .[/a-zA-Z0-9._]*")
    preprocess_reg = re.compile(r'preprocessing_time = [0-9]*.[0-9]*')
    main_loop_reg = re.compile(r' main_loop_time = [0-9]*.[0-9]*')
    ops_reg = re.compile(r'basic_ops = [0-9]*')

    ops_array = ops_reg.findall(total_string)
    time_array = time_reg.findall(total_string)
    ans_array = ans_reg.findall(total_string)
    input_array = input_reg.findall(total_string)
    name_array = name_reg.findall(total_string)
    preprocess_array = preprocess_reg.findall(total_string)
    main_loop_array = main_loop_reg.findall(total_string)
    eps_array = eps_reg.findall(total_string)
    
    if not v07:
        s_array = sort_reg.findall(total_string)

    for index, item in enumerate(time_array):
        if v07:
            print str(name_array[index][name_array[index].rfind('/')+1:])+'_v07,',
        else:
            print str(name_array[index][name_array[index].rfind('/')+1:])+',',

        
        input_list = input_array[index].split()
        rows = input_list[input_list.index("ROWS:")+1]
        cols =  input_list[input_list.index("COLUMNS:")+1]
        non_zeros = input_list[input_list.index("NON-ZEROS:")+1]
        density = float(non_zeros)/(float(rows)*float(cols))
        print str(rows)+',', str(cols)+',', str(non_zeros)+',', str(density)+',',
        time_list = time_array[index].split()
        print str(time_list[time_list.index("=")+1][:-1])+',',
        main_loop_list = main_loop_array[index].split()
        print str(main_loop_list[main_loop_list.index("=")+1][:-1])+',',
        preprocess_list = preprocess_array[index].split()
        print str(preprocess_list[preprocess_list.index("=")+1][:-1])+',',
        ans_list = ans_array[index].split()
        ans_list.reverse()
        ratio = ans_list[ans_list.index("=")-1]
        print str(ratio)+',',
        eps_list = eps_array[index].split()
        print eps_list[eps_list.index("=")+1]+',',
        ops_list = ops_array[index].split()
        print str(ops_list[ops_list.index('=')+1])+',',
        if not v07:
            s_list = s_array[index].split()
            print str(s_list[s_list.index("=")+1])
        else:
            print ','

def parse_glpk_output_file(file_name):
    try:
        my_file = open(file_name)
        total_string = my_file.read()
    except:
        print file_name + ', NOT FOUND'
        return

    time_reg = re.compile(r'[0-9]*.*[0-9]* secs')
    ans_reg = re.compile(r'obj =[ ]*[0-9]*.[0-9]*e[+-][0-9]*')
    input_reg = re.compile(r'[.]*: [0-9]* rows, [0-9]* columns, [0-9]* non-zeros') 
    name_reg = re.compile(r"[.]* reading problem data from `[/0-9a-zA-Z._]*'")

    time_array = time_reg.findall(total_string)
    ans_array = ans_reg.findall(total_string)
    input_array = input_reg.findall(total_string)
    name_array = name_reg.findall(total_string)

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

def main():
    args = sys.argv
    try:
        file_prefix = args[1]
    except:
        print 'Usage: python parse_output.py <file_prefix>'
        sys.exit(1)

    fp_file_name = './output/' + file_prefix + '_output'
    glpk_file_name = fp_file_name + '_glpk'
    v07_file_name = fp_file_name + '_v07'

    output_file_name = './output/' + file_prefix + '_run_stats.csv'
    sys.stdout = open(output_file_name, 'w')
    print "Filename,Rows,Columns,Nonzeros,Density,Time(s),Main Loop Time (s), Preprocessing Time (s), Ratio,Epsilon,basic_ops,SortRatio"

    parse_fastpc_output_file(fp_file_name, False)
    parse_glpk_output_file(glpk_file_name)
    parse_fastpc_output_file(v07_file_name, True)
    
main()
