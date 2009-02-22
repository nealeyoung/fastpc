import sys
import math

#The hop list
# My new data struct. Creates a doubly linked list.
# Except whenever you add a node you can get a reference to that node back
# You can then store the reference where ever you want
# You can use the reference to delete a node from the list
# The list can then be iterated over using the generate functions
# The iteration will hop over deleted nodes
# The iteration will be in the order the nodes were added
# So the iteration takes time in the order of the number of non deleted nodes
# If you delete a node it will also mark the location in the array used to store the reference as deleted
#I use all this functionality for dropping constriants
# with the lists being for iterating over coeffs in a column
# and the arrays I store the references to being the rows

#all nodes must be added before they are deleted
# wish I could make it so you could also use a reference to add in nodes to the list. It would be hard to keep the iteration in order though

#all 0(1)
class Node:

    class Reference:
        def __init__ (self,val,array,location,node):
            self.val = val
            self.array = array
            self.location = location
            self.node = node

        def delete(self):
            self.node.delete()


    def __init__ (self,val,array,location):
        ref= self.Reference(val,array,location,self)
        self.prev = None
        self.next = None
        self.ref = ref
        self.empty = False

    def add(self,val,array,location):
        new = Node(val,array,location)
        self.next = new
        new.prev = self
        return new
        
    def delete(self):
        if self.prev is None and self.next is None:
            self.ref.array[self.ref.location] = "deleted"
            self.empty = True
            return
        if self.prev is None:
            self.ref.array[self.ref.location] = "deleted"
            self.ref = self.next.ref
            self.next.ref.node = self
            self.next = self.next.next
            if self.next is not None:
                self.next.prev = self
            return
        if self.next is None:
            self.ref.array[self.ref.location] = "deleted"
            self.prev.next = None
            return

        self.ref.array[self.ref.location] = "deleted"
        self.prev.next = self.next
        self.next.prev = self.prev

    def generate(self):
        if self.empty:
            return
        yield self
        while self.next is not None:
            yield self.next
            self = self.next


    def gen_coeffs(self):
        for node in self.generate():
            yield node.ref.val
        
    def print_self(self):
        print 'linked list'
        for item in self.generate():
            print item.ref.val
        print ''

class Coeff:

    def __init__(self,value,variable,constraint):
        self.val = value
        self.var = variable
        self.cons = constraint

    def my_print(self):
        print 'value: ' + str(self.val) +' constraint: ' + str(self.cons) + ' vairable: ' + str(self.var)


#class contains,validates,and prints a problem instance 
class Problem:

    def __init__(self,epsilon,packing,covering,num_packing,num_covering,num_vars,packing_rhs,covering_rhs):

        self.check_cons(covering)
        self.check_cons(packing)
        self.check_eps(epsilon)
        self.check_num_vars(num_vars)
        self.eps = epsilon
        self.num_vars = num_vars
        self.p_cons_i,self.p_var_i = self.cons_index(packing,num_packing,num_vars) 
        self.c_cons_i,self.c_var_i = self.cons_index(covering,num_covering,num_vars)

        self.remove_empty()
        self.check_empty(self.c_cons_i)
        self.m = num_packing + num_covering
        self.p_rhs = packing_rhs
        self.c_rhs = covering_rhs
        self.create_active_set(num_covering)

        
    def create_active_set(self,num_covering):
        
        self.full_set = []
        start = Node(0,self.full_set,0)
        self.full_set.append(start.ref)
        end = start
        for i in range(1,num_covering):
            end = end.add(i,self.full_set,i)
            self.full_set.append(end.ref)

        self.active_set = start


    def cmp_coeff(self,a,b):
        if a.val > b.val:
            return 1
        if a.val < b.val:
            return -1
        return 0
        
    def check_num_vars(self,num_vars):
        if num_vars <= 0:
            print "invalid number of variables"
            sys.exit(1)

    def check_eps(self,eps):
        if eps >= 1 or eps <= 0:
            print "invalid epsilon value"
            sys.exit(1)

    def check_rhs(self,rhs):
        for item in rhs:
            if item <= 0:
                print "invalid rhs"
                sys.exit(1)

    def check_cons(self,coeffs):
        for item in coeffs:
            if item.val <= 0:
                print 'you have a non-positive coefficient'
                print 'leave out zero coeffs'
                sys.exit(1)

    def remove_empty(problem):
        for i,constraint in enumerate(problem.p_cons_i):
            if len(constraint) < 1:
                print 'removeing empty packing constraint'
                problem.p_cons_i.remove(i)
                problem.num_packing = problem.num_packing -1
                
    def check_empty(self,coeffs):
        for constraint in enumerate(coeffs):
            if len(constraint) < 1:
                print 'empty covering constraint this can not be satisfied'
                sys.exit(1)


    def cons_index(self,coeffs,num_cons,num_vars):
        #mlist is a list of lists of refs
        mlist = [ [] for item in range(num_cons)]
        #vlist is a list of linked lists
        vlist = [ None  for item in range(num_cons)]
        
        for coeff in coeffs:
            if coeff.cons >= num_cons:
                print 'there is a coefficient for a variable that is in a constraint the does not exist'
            else:
                if coeff.var>= num_vars:
                    print 'constraint has a coefficient for variable that does not esit'
                    continue
                if vlist[coeff.var] is None:
                    vlist[coeff.var] =Node(coeff,mlist[coeff.cons],len(mlist[coeff.cons]))
                    mlist[coeff.cons].append(vlist[coeff.var].ref)
                else:
                    end = vlist[coeff.var].add(coeff,mlist[coeff.cons],len(mlist[coeff.cons]))
                    mlist[coeff.cons].append(end.ref)

        return mlist,vlist

    #return max(Px) + 2*ln(m)/eps
    def set_N(self,x):
        self.N =max(eval_Px(self,x)) + (2*math.log(self.m)/self.eps)

    def scale_coeffs(self):
        for i,cons in enumerate(self.c_cons_i):
            for j,ref in enumerate(cons):
                self.c_cons_i[i][j].val.val = ref.val.val/self.c_rhs[i]*self.N

        for i,cons in enumerate(self.p_cons_i):
            for j,ref in enumerate(cons):
                self.p_cons_i[i][j].val.val = ref.val.val/self.p_rhs[i]*self.N

        
def skip(line):    
    if line == "\n": return True
    if line.split()[0][0:1] =='#': return True
    return False

#O(N)
def create_problem(file_name):    
    my_file = open(file_name,'r')
    packing = []
    covering = []
    packing_rhs = []
    covering_rhs = []
    pack_lines = False

    for line in my_file:
        if skip(line): continue
        if line.split()[0] == "covering_coeffs":
            break
        else:
            eps,num_pack_constraints,num_covering_constraints,num_variables = [float(x) for x in line.split()]

    for line in my_file:
        if skip(line): continue
        if line.split()[0] == "covering_constraint_values": break
        cons,variable,value = [float(x) for x in line.split()]
        covering.append(Coeff(value,int(variable),int(cons)))

    for line in my_file:
        if skip(line): continue
        if line.split()[0] == "packing_coeffs": break
        covering_rhs.append(float(line.split()[0]))

    for line in my_file:
        if skip(line): continue
        if line.split()[0] == "packing_constraint_values": break
        cons,variable,value = [float(x) for x in line.split()]
        packing.append(Coeff(value,int(variable),int(cons)))

    for line in my_file:
        if skip(line): continue
        packing_rhs.append(float(line.split()[0]))
        
    return Problem(eps,packing,covering,int(num_pack_constraints),int(num_covering_constraints),int(num_variables),packing_rhs,covering_rhs)

#all only called once
def init_x(num_vars):
    return [0 for x in range(num_vars)]

def evaluate_constraint(cons,x):
    return sum( [ref.val.val * x[ref.val.var] for ref in cons])

def eval_Px(prob,x):
    return map(lambda cons: evaluate_constraint(cons,x) if len(cons)>=1 else "deleted",prob.p_cons_i)
    
def eval_Cx(prob,x):
    return map(lambda cons: evaluate_constraint(cons,x) if len(cons)>=1 else "deleted",prob.c_cons_i)

#check this
#might have to return max ref instead of coeff
def max_coeff(top_node):
    max_coeff_val = 0
    max_coeff = None
    for coeff in top_node.gen_coeffs():
        if coeff.val > max_coeff_val:
            max_coeff_val = coeff.val
            max_coeff = coeff
    return max_coeff_val

#O(n) but only done m times for whole alg
def find_max_Ci(prob):
    return [ max_coeff(col) for col in prob.c_var_i]

#O(1)    
def find_max_Pi(prob):
    return [ max_coeff(col) for col in prob.p_var_i]

#O(n)
def min_pair(pairs):
    mmin = pairs[0][2]
    pair = pairs[0]
    for pair in pairs:
        if pair[2]<mmin:
            m_pair = pair
            mmin = pair[2]

    return pair

#O(N)
def find_incrament(prob,x):

    valid = []
    for j in range(prob.num_vars):
        if prob.c_var_i[j].ref.val is None:
            continue
        
        inc_amount = prob.eps/max([prob.max_Ci[j],prob.max_Pi[j]])
      
        jx_ratio = ratio(j,prob)
        if jx_ratio <= 1 + prob.eps:   
            valid.append((j,inc_amount,jx_ratio))
              
    if len(valid) < 1:
        print "Not satisfiable"
        print prob.num_rescale
        sys.exit(1)
    else:
        f_pair =  min_pair(valid)        
        return f_pair[0],f_pair[1]

#O(number of variables in col j)
def ratio(j,prob):    
    

    MijePx = [coeff.val*prob.ePx[coeff.cons] for coeff in prob.p_var_i[j].gen_coeffs()]
    partialjPx = sum(MijePx)/prob.sum_ePx #/prob.ePx_scale

   
    MijeCx = [coeff.val*math.e**(-prob.Cx[coeff.cons]) for coeff in prob.c_var_i[j].gen_coeffs()]
    partialjCx = (sum(MijeCx))/(prob.sum_eCx/prob.eCx_scale)

    #if all covering constraint for a variable are met we do not want to use it
    if partialjCx == 0:
        return 2
    
    
    # (mp/sp)/(mc/sc)
    return partialjPx/partialjCx

def create_ePx_eCx_and_sums(prob):
    prob.ePx = [ math.e**Pxi for Pxi in  prob.Px]
    prob.eCx = [math.e**(-Cxi) for Cxi in prob.Cx]
    prob.sum_ePx = sum(prob.ePx)
    prob.sum_eCx = sum(prob.eCx)
    

def rescale_up(prob):
    prob.num_rescale = prob.num_rescale + 1
    prob.eCx_scale = prob.eCx_scale * 100
    prob.eCx = [ x*100 for x in prob.eCx]
    prob.sum_eCx = non_drop_sum_eCx(prob)
    
def rescale_down(prob):
    prob.num_rescale = prob.num_rescale + 1
    prob.ePx_scale = prob.ePx_scale/100.0
    prob.ePx = [ x/100.0 for x in prob.ePx]
    prob.sum_ePx = sum(prob.ePx)
    

def rescale(prob):
    prob.num_rescale = prob.num_rescale + 1

    prob.eCx_scale = prob.eCx_scale * 1.0
    prob.eCx = [ x*1.0 for x in prob.eCx]
    prob.sum_eCx = non_drop_sum_eCx(prob)

    prob.ePx_scale = prob.ePx_scale/1.0
    prob.ePx = [ x/1.0 for x in prob.ePx]
    prob.sum_ePx = sum(prob.ePx)

def non_drop_sum_eCx(prob):
    return sum([ prob.eCx[place.ref.val] for place in prob.active_set.generate()]) 

def print_info(prob):
    print 'printing sum information for eCx'
    print 'current sum'
    print prob.sum_eCx
    print 'non_drop_sum(prob.eCx)'
    print non_drop_sum_eCx(prob)
    print 'iterations'
    print iterations
    print 'rescales'
    print prob.num_rescale
    

#O(1) minus rescales
def update_sum_eCx(i,inc_amount,prob):    
    if prob.sum_eCx < prob.rescale_lower_bound:
        rescale_up(prob)

    old = prob.eCx[i]
    new = math.e**(-prob.Cx[i])*prob.eCx_scale
    prob.sum_eCx = (prob.sum_eCx - old) + new 
    prob.eCx[i] = new
    
#O(1) mins rescales
def update_sum_ePx(i,inc_amount,prob):
    if prob.sum_ePx > prob.rescale_upper_bound:
        1#rescale(prob) 
    old = prob.ePx[i]
    new = (math.e**prob.Px[i])*prob.ePx_scale
    prob.sum_ePx = (prob.sum_ePx - old) + new 
    prob.ePx[i] = new

#O(width) but max of N over whole alg
def drop_cons(i,prob):
    print 'dropping constraint: ' + str(i)
    prob.num_del = prob.num_del + 1
    prob.full_set[i].delete()
    prob.sum_eCx = prob.sum_eCx - prob.eCx[i]
    
    for ref in prob.c_cons_i[i]:
        ref.delete()

#O(d) if you do not consider drop time and recalculating max
def inc_Px_Cx(j,inc_amount,prob):
    drop = False
    
    for coeff in prob.p_var_i[j].gen_coeffs():
        prob.Px[coeff.cons]= prob.Px[coeff.cons] + (coeff.val * inc_amount)
        update_sum_ePx(coeff.cons,inc_amount,prob)
 
    for coeff in prob.c_var_i[j].gen_coeffs():
        prob.Cx[coeff.cons]= prob.Cx[coeff.cons] + (coeff.val * inc_amount)
        update_sum_eCx(coeff.cons,inc_amount,prob)
        if prob.Cx[coeff.cons] > prob.N:
            drop_cons(coeff.cons,prob)
            drop = True
    if drop:
        prob.max_Ci = find_max_Ci(prob)
        if len(prob.c_cons_i) == prob.num_del :
            return True
    
    return False


def validate_packing(prob,solution):
    for Pxi in eval_Px(prob,solution):
        print Pxi
        print prob.N*(1+prob.eps)
        if Pxi > prob.N*(1+prob.eps):
            print "packing constraints violated"
            return
    print "packing constraints valid"

#except for perpaps some finding max issues and rescaling
#O((N+d)m*log(m))
# that is
#O(N*m*log(m))


#to do
#more testing
#back_up
#sorting
#alg 2
#update round robin style
#bucket sort index by variable matrix by value
#on updates only add to sums approximatly
#predict interations

if len(sys.argv) != 2:
    print "you should use one argument the specifies the file name for your program"
    sys.exit(1)

prob = create_problem(sys.argv[1])
x = init_x(prob.num_vars)
prob.set_N(x)
prob.scale_coeffs()
prob.max_Ci = find_max_Ci(prob)
prob.max_Pi = find_max_Pi(prob)
prob.rescale_lower_bound = 10
prob.rescale_upper_bound = 1000000000
prob.eCx_scale = 1.0
prob.ePx_scale = 1.0
prob.num_rescale = 0


neg_x = map(lambda x: -1*x,x)
prob.Cx = eval_Cx(prob,neg_x)
prob.Px = eval_Px(prob,x)
create_ePx_eCx_and_sums(prob)
prob.num_del = 0
iterations = 0

while True:
    
    iterations = iterations + 1
    j,inc_amount = find_incrament(prob,x)    
    all_deleted = inc_Px_Cx(j,inc_amount,prob)
    x[j] = x[j] + inc_amount
    if all_deleted: break
    #print 'iteration'
    #print iterations

print 'program finished'
print '-------------------------'
print '-------------------------'
print 'variables'
print x
print 'N'
print prob.N
print 'iterations'
print iterations
print 'rescales'
print prob.num_rescale

validate_packing(prob,x)









