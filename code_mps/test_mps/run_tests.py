import sys
import os

def main():
    both = False
    glpk_run = False
    fastpc_run = False
    
    args = sys.argv
    try:
        input_file_prefix = args[1]
        if len(args) < 3:
            both = True
        elif args[2] == 'glpk':
            glpk_run = True
        elif args[2] == 'fastpc':
            fastpc_run = True
    except:
        print 'no prefix spesified will run all files in directory'
        input_file_prefix = ''
        both = True

    # epsilons to be run for each input file generated        
    epsilons = [0.1]
    
    #each run is done with sort_ratio 1 (exact sorting)
    #and also for the sort_ratio defined here (approximate sorting)
    a_sort_ratio = 2 #if set to 0 or 1, only exact sorting run will be done

    output_file_name = 'output_'+input_file_prefix
    output_file_location = './output/'+output_file_name
    output_file_glpk_location = output_file_location + '_glpk'

    curr_dir = os.getcwd()
    fastpc_input_dir = curr_dir + '/test_cases'
    glpk_input_dir = curr_dir + '/test_cases_glpk/'
    fastpc_files = os.listdir(fastpc_input_dir)
    glpk_files = os.listdir(glpk_input_dir)
    glpk_command = "../../../glpk-4.30/examples/glpsol --cpxlp "

    if both or fastpc_run:
        for fp_file in fastpc_files:
            if fp_file.startswith(input_file_prefix) and not fp_file.startswith('.'):
                #for each file run test for different values of eps
                for eps in epsilons:
                    print eps, ' ', fp_file
                    #run the test and store the output

                    #run with sort_ratio = 1 (exact sorting)
                    cmd_exact = '../fastpc' + ' ' + str(eps) + ' ./test_cases/' + fp_file + ' 1 >> ' + output_file_location
                    os.system(cmd_exact)
                    if (a_sort_ratio > 1):
                        #run with sort_ratio > 1 (approx sorting)
                        cmd_approx = '../fastpc' + ' ' + str(eps) + ' ./test_cases/' + fp_file + ' ' + str(a_sort_ratio) + ' >> ' + output_file_location
                        os.system(cmd_approx)
                        
                glpk_file = fp_file + '_glpk'
                if glpk_file in glpk_files and both:
                    print glpk_file
                    os.system(glpk_command + glpk_input_dir+ glpk_file + ' >> ' + output_file_glpk_location)


    if glpk_run and not both:
        for glpk_file in glpk_files:
            if glpk_file.startswith(input_file_prefix) and not glpk_file.startswith('.'):
                print glpk_file
                
                os.system(glpk_command + glpk_input_dir+glpk_file + ' >> ' + output_file_glpk_location)
        

main()
