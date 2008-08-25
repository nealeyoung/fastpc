import sys
import os

def print_help():
    print "Usage: python run_tests.py [input_file_prefix] [-fastpc=false] [-v07=false] [-glpk=false] [-profile=true]"
    print "     -fastpc: By default fastpc is run. To NOT run fastpc, use -fastpc=false"
    print "     -v07: By default v07 is run. To NOT run v07, use -v07=false"
    print "     -glpk: By default glpk is run. To NOT run glpk, use -glpk=false"
    print "     -profile: By default the profiles are not created. To create profiles use -profile=true"
    print "     To use -profile=true option, make sure the compiler settings are set right to create gmon.out file for fastpc and version_2007"

def main():

    glpk_run = True
    fastpc_run = True
    v07_run = True
    profile = False
    input_file_prefix = ''

    try:
        args = sys.argv        
        l = len(args)
        if l == 1:
            fastpc_run = True
            glpk_run = True
            v07_run = True
        else:
            for i in range(1,l):
                arg = args[i]
                if arg == '--help':
                    sys.exit()
                elif arg.startswith('fastpc') and arg[8:] == 'false':
                    fastpc_run = False
                elif arg.startswith('v07') and arg[5:] == 'false':
                    v07_run == False
                elif arg.startswith('glpk') and arg[6:] == 'false':
                    glpk_run == False
                elif arg.startswith('profile') and arg[9:] == 'true':
                    profile = True
                else:
                    input_file_prefix = arg
    except:
        print_help()
        sys.exit()

    # epsilons to be run for each input file generated        
    epsilons = [0.1, 0.05]
    
    # sort_ratio to be used for runs
    sort_ratios = [1, 2]

    output_file_name = input_file_prefix + '_output'
    output_file_location = './output/' + output_file_name
    output_file_glpk_location = output_file_location + '_glpk'
    output_file_v07_location = output_file_location + '_v07'

    curr_dir = os.getcwd()
    fp_input_dir = curr_dir + '/test_cases/'
    glpk_input_dir = curr_dir + '/test_cases_glpk/'
    #v07_input_dir = curr_dir + '/test_cases_v07/'
    fastpc_files = os.listdir(fp_input_dir)
    glpk_files = os.listdir(glpk_input_dir)
    #v07_files = os.listdir(v07_input_dir)
    glpk_command = "../../../glpk/glpk-4.??/examples/glpsol --cpxlp " #works with glpk 4.x
    v07_command = '../../version_2007/code/fastpc '

    if fastpc_run:
        for fp_file in fastpc_files:
            if fp_file.startswith(input_file_prefix) and not fp_file.startswith('.'):
                #for each file run test for different values of eps
                for eps in epsilons:
                    # for each input file and eps, run for all sort_ratios
                    for sort_ratio in sort_ratios:
                        print 'fastpc: ', eps, ' ', fp_file, ' ', sort_ratio
                        cmd_exact = '../fastpc' + ' ' + str(eps) + ' ' + fp_input_dir + fp_file + ' ' + str(sort_ratio) +  ' >> ' + output_file_location
                        os.system(cmd_exact)

                    if profile:
		        #profile each run, both regular and line-by-line.
                        cmd_prof = 'gprof ../fastpc > ./profile/' + fp_file + '_prof.txt'
                        cmd_prof_line = 'gprof -l ../fastpc > ./profile/' + fp_file + '_prof_line.txt'
                        os.system(cmd_prof) 
                        os.system(cmd_prof_line)

                    # if v07_run: run v07 for eps and v07 input_file
                    if v07_run:
                        print 'version_2007: ', eps, ' ',  fp_file
                        os.system(v07_command + str(eps) + ' ' + fp_input_dir + fp_file + ' >> ' + output_file_v07_location)
                        if profile:
                            #profile each run, both regular and line-by-line.
                            cmd_prof = 'gprof ../../version_2007/code/fastpc > ./profile/' + fp_file + '_v07_prof.txt'
                            cmd_prof_line = 'gprof -l ../../version_2007/code/fastpc > ./profile/' + fp_file + '_v07_prof_line.txt'
                            os.system(cmd_prof) 
                            os.system(cmd_prof_line)

                # if glpk_run: run glpk for the glpk input file
                glpk_file = fp_file + '_glpk'
                if glpk_file in glpk_files and glpk_run:
                    print 'GLPK: ', glpk_file
                    os.system(glpk_command + glpk_input_dir + glpk_file + ' >> ' + output_file_glpk_location)
    else: #run glpk and/or v07 based on flags
        if glpk_run:
            for glpk_file in glpk_files:
                if glpk_file.startswith(input_file_prefix) and not glpk_file.startswith('.'):
                    print 'GLPK: ', glpk_file
                    os.system(glpk_command + glpk_input_dir+glpk_file + ' >> ' + output_file_glpk_location)
        if v07_run:
            for v07_file in fastpc_files:
                if v07_file.startswith(input_file_prefix) and not v07_file.startswith('.'):
                    for eps in epsilons:
                        print 'version_2007: ', eps, ' ',  v07_file
                        os.system(v07_command + str(eps) + ' ' + fp_input_dir + v07_file + ' >> ' + output_file_v07_location)
                        if profile:
                            #profile each run, both regular and line-by-line.
                            cmd_prof = 'gprof ../../version_2007/code/fastpc > ./profile/' + fp_file + '_v07_prof.txt'
                            cmd_prof_line = 'gprof -l ../../version_2007/code/fastpc > ./profile/' + fp_file + '_v07_prof_line.txt'
                            os.system(cmd_prof) 
                            os.system(cmd_prof_line)

main()
