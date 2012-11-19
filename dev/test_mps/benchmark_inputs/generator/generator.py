import sys
import random as R
import math

def entry(con, var):
    return str(con) + ' ' + str(var) + ' 1\n'

def generate_instance(n, outfile):
    alpha = R.uniform(0.1, 0.25)
    p = R.uniform(0.2, 0.7)
    r = R.uniform(0.05, 0.15)

    m = int(r * n * math.log(n))

    ntoalpha = int(math.pow(n, alpha))
    num_subsets = n * ntoalpha
    pop = ntoalpha * ntoalpha
    delta = int(p * pop)

    print 'Generating instance with following parameters'
    print '\t alpha = ', alpha
    print '\t p = ', p
    print '\t r = ', r
    print '\t n = ', n
    print '\t delta = ', delta

    #empty first line (values will be added here later)
    outfile.write('                                                   \n')

    # first set of constraints (one for each group)
    # we put the same element in all subsets in a group
    con = 0
    nz = 0
    for i in xrange(n):
        for j in xrange(ntoalpha):
            outfile.write(entry(con, ntoalpha * i + j))
            nz += 1
        con += 1

    # pick a random solution
    sol = []
    for i in xrange(n):
        sol.append(R.randrange(ntoalpha))

    # remaining constraints
    # running step 2 of the model in loop
    # if two subsets appear in the solution above, don't put a common element in them
    for i in xrange(m-1):
        # constraint
        g1 = R.randrange(n)
        g2 = R.randrange(n)
        S = R.sample(range(pop), delta)
        for s in S:
            x1 = s // ntoalpha
            x2 = s % ntoalpha
            if sol[g1] == x1 and sol[g2] == x2:
                continue
            outfile.write(entry(con, ntoalpha * g1 + x1))
            outfile.write(entry(con, ntoalpha * g2 + x2))
            nz += 2
            con += 1
    outfile.seek(0)
    outfile.write(str(con) + ' ' + str(num_subsets) + ' ' + str(nz))
        
def main():
    args = sys.argv
    if len(args) < 3:
        print 'Usage: python generator.py <number of variables> <output_file> [seed]'
        print '(seed is optional)'
        return
    n = int(args[1]) #number of variables
    outfile = open(args[2], 'w')
    if len(args) > 3:
        s = int(args[3])
        R.seed(s)
    generate_instance(n, outfile)
    outfile.close()

main()
