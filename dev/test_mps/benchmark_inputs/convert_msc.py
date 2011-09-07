import sys

usage = 'python convert_msc.py <msc_file_path>'

try:
    msc_path = sys.argv[1]
    msc_file = open(msc_path)
except:
    print usage
    exit()

base_name = msc_path[msc_path.rindex('/') + 1 : msc_path.index('.msc')]

fastpc = open(base_name + '_fastpc_input', 'w')

c = 0
total = 0

fastpc.write('                                     \n')

for line in msc_file:
    if line.startswith('p'):
        arr = line.split()
        U = arr[2]
        S = arr[3]
    elif line.startswith('s'):
        arr = line.split()
        l = len(arr)
        i = 1
        while i < l:
            #constraint
            r = arr[i]
            entry = r + ' ' + str(c) + ' 1\n'
            fastpc.write(entry)
            i += 1
            total += 1
        c += 1

header = U + ' ' + S + ' ' + str(total)
fastpc.seek(0)
fastpc.write(header)

fastpc.close()
msc_file.close()
