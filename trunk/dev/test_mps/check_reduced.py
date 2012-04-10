import sys

args = sys.argv

if len(args) > 1:
    low = float(sys.argv[1])
else:
    low = 0.9

high = 1/low

input_params = {}

processed = set()
all_files = set()

def load_input_params():
    param_file = open('parsed_input_params.txt')
    for line in param_file:
        arr = line.split('$')
        input_params[arr[0].split('#')[1]] = arr[1]

def significant(*val):
    for v in val:
        if v <= low or v >= high:
            return True
    return False

def check_reduced(prefix):
    cplex_file = open('output_cplex/' + prefix + '_output_cplex')

    file_name = ''

    for line in cplex_file:
        if line.startswith("File type: Problem") or line.startswith("Problem"): # New run
            file_name = line[line.rindex('/')+1 : line.rindex('\'')]
            all_files.add(file_name)
        if line.startswith('Reduced LP has') and len(file_name) > 0:
            if file_name in processed:
                continue
            arr = line.split()
            rrows = int(arr[arr.index('rows,') - 1])
            rcols = int(arr[arr.index('columns,') - 1])
            rnz = int(arr[arr.index('nonzeros.') - 1])

            input_list = file_name.split('_')
            input_list[-1] = ''
            fastpc_file_name = '_'.join(input_list)[:-1]
            orig = map(float, input_params.get(fastpc_file_name).split('#'))
            orows = orig[1]
            ocols = orig[0]
            onz = orig[2]

            if rrows > orows or rcols > ocols:
                temp = orows
                orows = ocols
                ocols = temp

            if significant(rrows/orows, rcols/ocols, rnz/onz):
                print file_name
                print '\toriginal:\t', int(orows), '\t', int(ocols), '\t', int(onz)
                print '\treduced :\t', rrows, '\t', rcols, '\t', rnz
                print '\tratio   :\t%(rows).3f \t%(cols).3f\t%(nz).3f' % {'rows' : rrows/orows, 'cols' : rcols/ocols, 'nz' : rnz/onz}
                processed.add(file_name)
            file_name = ''

def main():
    load_input_params()
    prefixes = ['DIMACS', 'frb', '2011_Nov25']
    for prefix in prefixes:
        check_reduced(prefix)

    print '\n-------------------------------------'
    print 'Total files:', len(all_files)
    print 'Files with reduction:', len(processed)
    print '-------------------------------------\n'

main()
