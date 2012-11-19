import sys
from os import system as S

def main():
    args = sys.argv

    if len(args) < 2:
        print 'Usage: python generate_input_set <input_identifier> [seed]'
        print 'seed is optional'

    identifier = args[1]
    
    seed = None
    if len(args) > 2:
        seed = int(args[2])
        
    n_array = [2000, 4000, 8000, 12000, 16000, 20000]

    count = 0
    for n in n_array:
        filename = identifier + '_' + str(count) + '_input'
        cmd = 'python generator.py ' + str(n) + ' ' + filename
        if seed != None:
            cmd += ' ' + str(seed)
        S(cmd)
        glpk_filename = filename + '_glpk'
        S('python ../../convert_fastpc_to_glpk.py ' + filename + ' ' + glpk_filename)
        count += 1

main()
