import sys
import math

#This class is a little weird.
#It is basically a doubly linked list
#Except that any value added to it is given a link to the node that it is added as part of.
#The idea is to build the list by adding elements you are also adding to an array. 
#Then when an element of the array is marked as deleted.
#You delete it from the linked list by following the link into it.
#This allows you to continue to access the array by the same indexes
#But if you want to iterate over the array skipping the invalid indexes
#You can by instead iterating over the list using the generator function

#all 0(1)
class Node:
    def __init__(self,prev,next,val):
        self.prev = prev
        self.next = next
        self.val = val

    def add(self,coeff):
        if self.val == None:
            self.val = coeff
            self.val.node = self
            return self
        else:
            new = Node(self,None,coeff)
            new.val.node = new
            self.next = new
            return new

    def generate(self):
        current = self
        while True:
            if current is None:
                break

            yield current.val

            if current.next is not None:
                current = current.next
            else:
                break

    def remove(self):
        if self.prev is None and self.next is None:
            self.val = None
            return
        
        if self.next is None:
            self.prev.next = None
            return
        
        if self.prev is None:
            self.val = self.next.val
            self.next = self.next.next
            return
        
        self.prev.next = self.next
        self.next.prev = self.prev

class Coeff:
    def __init__(self,value,variable,constraint):
        self.val = value
        self.var = variable
        self.cons = constraint

    def my_print(self):
        print 'value: ' + str(self.val) +' constraint: ' + str(self.cons) + ' vairable: ' + str(self.var)

class Set_Mark:
    def __init__(self,value):
        self.val = value

#class contains,validates,and prints a problem instance 
class Problem:
    def __init__(self,epsilon,packing,covering,num_packing,num_covering,num_vars,packing_rhs,covering_rhs):
        self.check_cons(covering)
        self.check_cons(packing)
        self.check_eps(epsilon)
        self.check_num_vars(num_vars)
        self.eps = epsilon
        self.num_vars = num_vars
        self.p_var_i = self.var_index(packing,num_vars)
        self.p_cons_i = self.cons_index(packing,num_packing) 
        self.c_var_i = self.var_index(covering,num_vars)
        self.c_cons_i = self.cons_index(covering,num_covering)
        self.remove_empty()
        self.check_empty(self.c_cons_i)
        self.m = num_packing + num_covering
        self.p_rhs = packing_rhs
        self.c_rhs = covering_rhs
        self.p_var_i = self.val_index_list(self.p_var_i)
        self.c_var_i = self.val_index_list(self.c_var_i)
        self.create_active_set(num_covering)
        
    def create_active_set(self,num_covering):
        start = Node(None,None,None)
        end = start
        self.full_set =[]
        for i in range(num_covering):
            new = Set_Mark(i)
            self.full_set.append(new)
            end.add(new)
        
        self.active_set = start
        
    def val_index_list(self,m_var_i):
        for j,col in enumerate(m_var_i):
            col.sort(self.cmp_coeff)
            start = Node(None,None,None)
            end = start
            for coeff in col:
                end = end.add(coeff)
            m_var_i[j] = start

        return m_var_i

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

    def var_index(self,coeffs,num_vars):
        m_list = [ [] for item in range(num_vars)]
        for item in coeffs:
            if item.var >= num_vars:
                print 'constraint has a coefficient for variable that does not esit'
            else:
                m_list[item.var].append(item)
        return m_list

    def cons_index(self,coeffs,num_cons):
        m_list = [ [] for item in range(num_cons)]
        for item in coeffs:
            if item.cons >= num_cons:
                print 'there is a coefficient for a variable that is in a constraint the does not exist'
            else:
                m_list[item.cons].append(item)
        return m_list

    #return max(Px) + 2*ln(m)/eps
    def set_N(self,x):
        self.N =max(eval_Px(self,x)) + (2*math.log(self.m)/self.eps)

    def scale_coeffs(self):
        for i,cons in enumerate(self.c_cons_i):
            for j,coeff in enumerate(cons):
                self.c_cons_i[i][j].val = coeff.val/self.c_rhs[i]*self.N

        for i,cons in enumerate(self.p_cons_i):
            for j,coeff in enumerate(cons):
                self.p_cons_i[i][j].val = coeff.val/self.p_rhs[i]*self.N

        
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
    return sum( [coeff.val * x[coeff.var] for coeff in cons])

def eval_Px(prob,x):
    return map(lambda cons: evaluate_constraint(cons,x) if len(cons)>=1 else "deleted",prob.p_cons_i)
    
def eval_Cx(prob,x):
    return map(lambda cons: evaluate_constraint(cons,x) if len(cons)>=1 else "deleted",prob.c_cons_i)

#check this
def max_coeff(node):
    if node is None:
        print "list is empty when finding max coeff"
        return Coeff(-1,-1,-1)
    else:
        return node.val

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

#O(m*n)
def find_incrament(prob,x):

    valid = []
    for j in range(prob.num_vars):
        if prob.c_var_i[j].val is None:
            continue
    
        inc_amount = prob.eps/max([prob.max_Ci[j].val,prob.max_Pi[j].val])
      
        jx_ratio = ratio(j,prob)
        if jx_ratio <= 1 + prob.eps:   
            valid.append((j,inc_amount,jx_ratio))
              
    if len(valid) < 1:
        print "Not satisfiable"
        print "N"
        print prob.N
        sys.exit(1)
    else:
        f_pair =  min_pair(valid)
        
        return f_pair[0],f_pair[1]

#O(m)
def ratio(j,prob):

    ePx = [ math.e**Pxi for Pxi in  prob.Px]
    MijePx = [coeff.val*ePx[coeff.cons] for coeff in prob.p_var_i[j].generate()]
    partialjPx = sum(MijePx)/sum(ePx)

    eCx = [math.e**(-prob.Cx[place.val]) for place in prob.active_set.generate()]
    MijeCx = [coeff.val* math.e**(-prob.Cx[coeff.cons]) for coeff in prob.c_var_i[j].generate()]
    partialjCx = sum(MijeCx)/sum(eCx)

    return partialjPx/partialjCx

#O(width) but max of N over whole alg
def drop_cons(i,prob):
    prob.num_del = prob.num_del + 1
    prob.full_set[i].node.remove()
    for coeff in prob.c_cons_i[i]:
        coeff.node.remove()
    prob.c_cons_i[i] = "deleted" 

#O(d) if you do not consider drop time and recalculating max
def inc_Px_Cx(j,inc_amount,prob):
    drop = False
    
    for coeff in prob.p_var_i[j].generate():
        prob.Px[coeff.cons]= prob.Px[coeff.cons] + (coeff.val * inc_amount)
            
    for coeff in prob.c_var_i[j].generate():
        prob.Cx[coeff.cons]= prob.Cx[coeff.cons] + (coeff.val * inc_amount)
        if prob.Cx[coeff.cons] > prob.N:
            drop_cons(coeff.cons,prob)
            drop = True
    if drop:
        prob.max_Ci = find_max_Ci(prob)
        if len(prob.c_cons_i) == prob.num_del :
            return True
    
    return False

#O((m*n)*m*log(m))

#to do
#more testing
#back_up
####################
#deletes
#make M_var_i into array of sorted by value doublely linked lists
#the items of the lists are coeffs with pointers into their location in the list
#Max is simply top of the lists
#delete is called when you find a Cix > N when you do an update
#you take C_cons_i[i] and go through each coeff
#delete it from its doubly linked list
#thats all you have to do.
#################################

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


neg_x = map(lambda x: -1*x,x)
prob.Cx = eval_Cx(prob,neg_x)
prob.Px = eval_Px(prob,x)

prob.num_del = 0
iterations = 0

while True:
    
    iterations = iterations + 1
    j,inc_amount = find_incrament(prob,x)    
    all_deleted = inc_Px_Cx(j,inc_amount,prob)
    x[j] = x[j] + inc_amount
    if all_deleted: break
    
print 'variables'
print x
print 'N'
print prob.N
print 'iterations'
print iterations












