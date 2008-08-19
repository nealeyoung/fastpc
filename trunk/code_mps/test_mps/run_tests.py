import sys
import os

def main():

    args = sys.argv
    try:
        input_file_prefix = args[1]
    except:
        input_file_prefix = ''

    # epsilons to be run for each input file generated        
    epsilons = [0.1, 0.01]
    
    #each run is done with sort_ratio 1 (exact sorting)
    #and also for the sort_ratio defined here (approximate sorting)
    a_sort_ratio = 2 #if set to 0 or 1, only exact sorting run will be done

    output_file_name = 'output_test_m'
    output_file = open('./output/'+output_file_name, 'w')

    curr_dir = os.getcwd()
    fastpc_input_dir = curr_dir + '/test_cases'
    glpk_input_dir = curr_dir + '/test_cases_glpk'
    fastpc_files = os.listdir(fastpc_input_dir)

    for fp_file in fastpc_files:
        if fp_file.startswith(input_file_prefix):
            #for each file run test for different values of eps
            for eps in epsilons:
                print eps, ' ', fp_file
                #run the test and store the output

                #run with sort_ratio = 1 (exact sorting)
                cmd_exact = '../fastpc' + ' ' + str(eps) + ' ./test_cases/' + fp_file + ' 1 >> ' + output_file_name
                os.system(cmd_exact)
                if (a_sort_ratio > 1):
                    #run with sort_ratio > 1 (approx sorting)
                    cmd_approx = '../fastpc' + ' ' + str(eps) + ' ./test_cases/' + fp_file + ' ' + str(a_sort_ratio) + ' >> ' + output_file_name
                    os.system(cmd_approx)

                #do the runs for GLPK also
                
    output_file.close()

main()
