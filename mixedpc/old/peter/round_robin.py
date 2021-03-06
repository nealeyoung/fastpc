import sys
import math

###################
#Main data structs
###################

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
        self.ref= self.Reference(val,array,location,self)
        self.prev = None
        self.next = None
        self.empty = False

    def add(self,val,array,location):
        new = Node(val,array,location)
        self.next = new
        new.prev = self
        return new
        
    def delete(self):
        if self.prev is None and self.next is None:
            print 'var deleted'
            self.j_link.delete()
            self.empty = True
            return
        if self.prev is None:
            self.ref = self.next.ref
            self.next.ref.node = self
            self.next = self.next.next
            if self.next is not None:
                self.next.prev = self
            return
        if self.next is None:
            self.prev.next = None
            return

        self.prev.next = self.next
        self.next.prev = self.prev

    def generate(self):
        if self.empty or self is None:
            return
        yield self
        current = self.next
        
        while current is not None and not current.empty:
            yield current
            current = current.next
  
    def gen_val(self):
        for node in self.generate():
            yield node.ref.val

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


######################
#Problem Set up stuff
######################

#class contains,validates,and prints a problem instance 
class Problem:

    def __init__(self,epsilon,packing,covering,num_packing,num_covering,num_vars,packing_rhs,covering_rhs):

        self.check_cons(covering)
        self.check_cons(packing)
        self.check_eps(epsilon)
        self.check_num_vars(num_vars)
        print num_vars
        self.eps = epsilon
        self.num_vars = num_vars
        packing = self.basic_coeff_sort(packing,num_vars)
        covering = self.basic_coeff_sort(covering,num_vars)
        
        self.p_cons_i,self.p_var_i = self.cons_index(packing,num_packing,num_vars) 
        self.c_cons_i,self.c_var_i = self.cons_index(covering,num_covering,num_vars)

        self.m = (num_packing + num_covering)
        self.remove_empty()
        self.check_empty(self.c_cons_i)

        
        self.p_rhs = packing_rhs
        self.c_rhs = covering_rhs
        self.create_active_set(num_covering)
        self.j_link()
        
    def create_active_set(self,num_covering):
        
        self.full_set = []
        start = Node(0,self.full_set,0)
        self.full_set.append(start.ref)
        end = start
        for i in range(1,num_covering):
            end = end.add(i,self.full_set,i)
            self.full_set.append(end.ref)

        self.active_set = start

    def j_link(self):
        start = Node(0,self.c_var_i[0],0)
        self.c_var_i[0].j_link = start.ref
        end = start
        for i, list_head in enumerate(self.c_var_i[1:]):
            end=end.add(i+1,list_head,i+1)
            list_head.j_link = end.ref

        self.active_js = start


    def cmp_coeff(self,a,b):
        if a.val > b.val:
            return -1
        if a.val < b.val:
            return 1
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
                print i
                problem.p_cons_i[i] = []
                problem.m = problem.m -1
                
    def check_empty(self,coeffs):
        for constraint in enumerate(coeffs):
            if len(constraint) < 1:
                print 'empty covering constraint this can not be satisfied'
                sys.exit(1)



    def basic_coeff_sort(self,coeffs,num_vars):
        cols = [[] for item in range(num_vars)]
        new_coeffs = []
        for coeff in coeffs:
            cols[coeff.var].append(coeff)

        map(lambda col: col.sort(self.cmp_coeff),cols)
 
        for col in cols:
            for coeff in col:
                new_coeffs.append(coeff)
        return new_coeffs

    def cons_index(self,coeffs,num_cons,num_vars):
        #mlist is a list of lists of refs
        print 'cons index'
        mlist = [ [] for item in range(num_cons)]

        #vlist is a list of linked lists
        vlist = [ None  for item in range(num_vars)]
        ends = [None for item in range(num_vars)]
        
        for coeff in coeffs:
            if coeff.cons >= num_cons:
                print 'there is a coefficient for a variable that is in a constraint the does not exist'
            else:
                if coeff.var>= num_vars:
                    print 'constraint has a coefficient for variable that does not exist'
                    continue
                if vlist[coeff.var] is None:
                    vlist[coeff.var] =Node(coeff,mlist[coeff.cons],len(mlist[coeff.cons]))
                    ends[coeff.var] = vlist[coeff.var]
                    mlist[coeff.cons].append(vlist[coeff.var].ref)
                else:
                    ends[coeff.var] = ends[coeff.var].add(coeff,mlist[coeff.cons],len(mlist[coeff.cons]))
                    mlist[coeff.cons].append(ends[coeff.var].ref)

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

#############################
# Main alg helper functions
#############################

#all only called once
def init_x(num_vars):
    return [0 for x in range(num_vars)]

def evaluate_constraint(cons,x):
    return sum( [ref.val.val * x[ref.val.var] for ref in cons])

def eval_Px(prob,x):
    return map(lambda cons: evaluate_constraint(cons,x) if len(cons)>=1 else 0,prob.p_cons_i)
    
def eval_Cx(prob,x):
    return map(lambda cons: evaluate_constraint(cons,x) if len(cons)>=1 else 0,prob.c_cons_i)

#will need to check through the to ln possibilites
def max_coeff(top_node):
    return top_node.ref.val.val
##    #print 'find_max'
##    max_coeff_val = 0
##    max_coeff = None
##    for coeff in top_node.gen_coeffs():
##        if coeff.val > max_coeff_val:
##            #print 'max_found_late'
##            max_coeff_val = coeff.val
##            max_coeff = coeff
##    return max_coeff_val



#should be O(n) but only done m times for whole alg: but needs fixed
def find_max_Ci(prob):
    return [ max_coeff(col) for col in prob.c_var_i]


#should be O(1)

def validate_packing(prob,solution,factor):
    for Pxi in eval_Px(prob,solution):
        if Pxi > prob.N*(1+factor*prob.eps):
            print "packing constraints violated"
            return
    print "packing constraints valid"


def max_pack_break(prob,solution):
    print 'max ratio of amount a packing constraint is broken'
    print max(eval_Px(prob,solution))/prob.N

def find_max_Pi(prob):
    return [max_coeff(col)for col in prob.p_var_i]
    #return [ col.ref.val.val for col in prob.p_var_i]

def create_ePx_eCx_and_sums(prob):
    prob.ePx = [ math.e**Pxi for Pxi in  prob.Px]
    prob.eCx = [math.e**(Cxi) for Cxi in prob.Cx]
    prob.sum_ePx = sum(prob.ePx)
    prob.sum_eCx = sum([1/x for x in prob.eCx])


def non_drop_sum_eCx(prob):
    
    return sum([ (1/prob.eCx[place.ref.val]) for place in prob.active_set.generate()]) 


#O(1) minus rescales
def update_sum_eCx(i,inc_amount,prob):    

    old = prob.eCx[i]
    new = math.e**(prob.Cx[i])
    prob.sum_eCx = (prob.sum_eCx - (1/old)) + (1/new) 
    prob.eCx[i] = new
    
#O(1) minus rescales
def update_sum_ePx(i,inc_amount,prob):

    old = prob.ePx[i]
    new = (math.e**(prob.Px[i]))
    prob.sum_ePx = (prob.sum_ePx - old) + new 
    prob.ePx[i] = new

#O(width) but max of N over whole alg
def drop_cons(i,prob):
    print 'number of dropped constraints: ' + str(prob.num_del +1)
    prob.num_del = prob.num_del + 1
    prob.full_set[i].delete()
    prob.sum_eCx = prob.sum_eCx - (1/prob.eCx[i])
    prob.eCx[i] = 1
    for ref in prob.c_cons_i[i]:
        ref.delete()

def scale_Px(prob):

    prob.num_rescale = prob.num_rescale + 1
    prob.ePx = [x/prob.Px_scale for x in prob.ePx]
    prob.sum_ePx = sum(prob.ePx)

    for i,x in enumerate(prob.ePx):
        if x > 0: #check this if
            prob.Px[i] = math.log(x)
            

def scale_Cx(prob):

    prob.num_rescale = prob.num_rescale + 1
    prob.eCx = [x/prob.Cx_scale for x in prob.eCx]
    prob.sum_eCx = non_drop_sum_eCx(prob)

    for i,x in enumerate(prob.eCx):
        if x > 0: #check this if
            prob.Cx[i] = math.log(x)    



###########################################
#main functions
###########################################

def do_phase(prob,x):

    #used for making sure at least one increment is done
    not_valid = True 
    prob.other_skip = 0
    #loop over each variable
    for j in prob.active_js.gen_val():

        #loop incrementing on  one variable
        while True:

            inc_amount = (prob.eps/max([ find_max_Ci(prob)[j], find_max_Pi(prob)[j]]))
            
            jx_ratio = ratio(j,prob)
            
            prob.checks = prob.checks + 1
            
            #check if we can  increment this variable
            if jx_ratio <= ((1 + prob.eps)**2)*2:  
                not_valid = False

                #update sum_ePx sum_eCx Cx Px ePx eCx and delete met constraints
                all_deleted = inc_Px_Cx(j,inc_amount,prob)
                prob.adds = prob.adds + 1

                #update x for final output
                x[j] = x[j] + inc_amount
                
                if all_deleted:
                    #if all constraints are deleted exit program
                    return True,x
            else:
                break

    if not_valid:
        print "Not satisfiable"
        #print prob.Cx[[x.ref.val for x in prob.active_set.generate()][0]]
        print prob.num_del
        sys.exit(1)
    else:
        return False,x

#O(number of variables in col j)
def ratio(j,prob):    
    
    MijePx = [coeff.val*prob.ePx[coeff.cons] for coeff in prob.p_var_i[j].gen_coeffs()]
    partialjPx = sum(MijePx)/prob.sum_ePx #/prob.ePx_scale
   
    MijeCx = [coeff.val*(1/prob.eCx[coeff.cons]) for coeff in prob.c_var_i[j].gen_coeffs()]
    #partialjCx = (sum(MijeCx))/non_drop_sum_eCx(prob)
    partialjCx = (sum(MijeCx))/prob.sum_eCx

    if partialjCx == 0:
        return 100
    
    return partialjPx/partialjCx

            

#O(d) if you do not consider drop time and recalculating max
def inc_Px_Cx(j,inc_amount,prob):
    drop = False
    
    for coeff in prob.p_var_i[j].gen_coeffs():
        if prob.Px[coeff.cons] > prob.Px_bound:
            scale_Px(prob)

        prob.Px[coeff.cons]= prob.Px[coeff.cons] + ((coeff.val * inc_amount))
 
        update_sum_ePx(coeff.cons,inc_amount,prob)
 
    for coeff in prob.c_var_i[j].gen_coeffs():
        if prob.Cx[coeff.cons] > prob.Cx_bound:
            print 'rescale to prevent precision problems on eCx'
            scale_Cx(prob)
        prob.Cx_real[coeff.cons] = prob.Cx_real[coeff.cons] + (coeff.val * inc_amount)
        prob.Cx[coeff.cons]= prob.Cx[coeff.cons] + (coeff.val * inc_amount)
        update_sum_eCx(coeff.cons,inc_amount,prob)

        if prob.Cx_real[coeff.cons] > prob.N:

            drop_cons(coeff.cons,prob)
            drop = True

    if drop:

        if len(prob.c_cons_i) == prob.num_del+1 :
            return True
    
    return False




#to do
#more testing
#test current sorting and deleting
#compartmentalize data struct
#look at paper
#
#
#
#sorting so we can find max coeffs fast
#bucket sort index by variable matrix by value
#on updates only add to sums approximatly



#########################
# Set up and main alg
#########################



def run():
    import psyco
    psyco.full()


    if len(sys.argv) != 2:
        print "you should use one argument the specifies the file name for your program"
        sys.exit(1)

    prob = create_problem(sys.argv[1])
    x = init_x(prob.num_vars)

    prob.set_N(x)
    prob.scale_coeffs()

    prob.Px_bound = 22
    prob.Cx_bound = 10

    prob.Px_scale = 1000000.0
    prob.Cx_scale = 10000.0

    prob.num_rescale = 0

    prob.checks = 0
    prob.adds = 0

    prob.Cx = eval_Cx(prob,x)
    prob.Cx_real = eval_Cx(prob,x)
    prob.Px = eval_Px(prob,x)
                                    
    create_ePx_eCx_and_sums(prob)

    prob.num_del = 0
    iterations = 0

    while True:
        
        iterations = iterations + 1
        print 'phase'
        print iterations
        all_deleted , x = do_phase(prob,x) 
        if all_deleted: break



    #####################################
    #solution,validation and stats
    ######################################

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



    print 'run time stuff'
    print 'm*log(m)/eps^2) number of incraments in worst case'
    print (prob.m  * math.log(prob.m))/prob.eps**2
    print 'actual increments'
    print prob.adds
    print '(n+m)*log(m)/eps^2 number of checks of ratio in worst case'
    print ( (prob.m+prob.num_vars) * math.log(prob.m))/prob.eps**2
    print 'actual checks'
    print prob.checks

    print 'remaining active variables'
    print len([x for item in prob.active_js.generate()])



            


    validate_packing(prob,x,1)
    max_pack_break(prob,x)

    out_file = open('fastpc_solution','w')

    for item in x:
        out_file.write(str(item/(max(x)/256)))
        out_file.write('\n')


run()
