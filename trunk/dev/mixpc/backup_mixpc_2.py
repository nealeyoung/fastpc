import sys
import math

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
        self.p_var_i = self.var_index(packing,num_vars)
        self.p_cons_i = self.cons_index(packing,num_packing) 
        self.c_var_i = self.var_index(covering,num_vars)
        self.c_cons_i = self.cons_index(covering,num_covering)
        self.remove_empty()
        self.check_empty(self.c_cons_i)
        self.m = num_packing + num_covering
        self.p_rhs = packing_rhs
        self.c_rhs = covering_rhs

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
    
    if line == "\n":
            return True

    if line.split()[0][0:1] =='#':
            return True

    return False

#return initialized problem
#coeffs list of lists of coeffs for each variable
#need to validate a
def create_problem(file_name):
    
    my_file = open(file_name,'r')
    
    eps = 0.05
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
            epsilon,num_pack_constraints,num_covering_constraints,num_variables = [float(x) for x in line.split()]

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


def init_x(num_vars):
    return [0 for x in range(num_vars)]

def evaluate_constraint(cons,x):
    return sum( [coeff.val * x[coeff.var] for coeff in cons])

def eval_Px(prob,x):
    return map(lambda cons: evaluate_constraint(cons,x) if len(cons)>=1 else "deleted",prob.p_cons_i)
    

def eval_Cx(prob,x):
    return map(lambda cons: evaluate_constraint(cons,x) if len(cons)>=1 else "deleted",prob.c_cons_i)

def max_coeff(mlist):
    
    mmax = 0
    if len(mlist) <= 0:
        print "list is empty when finding max coeff"
        return Coeff(-1,-1,-1)
    else:
        for i,coeff in enumerate(mlist):
            if coeff.val > mmax:
                mmax = coeff.val
                index = i
    return mlist[i]


#could be slow
def find_max_Ci(prob):
    return [ max_coeff(col) for col in prob.c_var_i]
    
def find_max_Pi(prob):
    return [ max_coeff(col) for col in prob.p_var_i]

def min_pair(pairs):
    mmin = pairs[0][2]
    pair = pairs[0]
    for pair in pairs:
        if pair[2]<mmin:
            m_pair = pair
            mmin = pair[2]

    return pair
    

def find_incrament(prob,x):
    valid = []
    neg_x = map(lambda x: -1*x,x)
    Cx = eval_Cx(prob,neg_x)
    Px = eval_Px(prob,x)

    for j in range(prob.num_vars):
        if len(prob.c_var_i[j]) < 1 or len(prob.p_var_i[j])<1:
            continue
    
        inc_amount = prob.eps/max([prob.max_Ci[j].val,prob.max_Pi[j].val])
      
        if ratio(j,x,prob,Px,Cx) <= 1 + prob.eps:   
            valid.append((j,inc_amount,ratio(j,x,prob,Px,Cx)))
    

        #if len(prob.c_cons_i) < 1:
        #    print 'weird'
        #    valid.append( (j,0))
        
      
    if len(valid) < 1:
        print "Not satisfiable"
        print "N"
        print prob.N
        sys.exit(1)
    else:
        f_pair =  min_pair(valid)
        
        return f_pair[0],f_pair[1]



def delete_one_c_cons(prob,i):
    
    prob.c_cons_i[i] = []

    for j,col in enumerate(prob.c_var_i):
        for index,coeff in enumerate(col):
            if coeff.cons == i:
                prob.c_var_i[j].pop(index)

def delete_cov_constraints(prob,x):
    for i,cons in enumerate(prob.c_cons_i):
        if evaluate_constraint(cons,x) >= prob.N:
            print 'constraint: ' +str(i) + ' deleted'
            delete_one_c_cons(prob,i)
            prob.num_del = prob.num_del + 1
    
    if len(prob.c_cons_i) != prob.num_del :
        prob.min_CX = min(eval_Cx(prob,x))
        prob.max_Ci = find_max_Ci(prob)
        return False
    else:
        return True
    

def my_sum(mlist):
    return reduce(lambda accum,y: accum+y if y != "deleted" else accum,mlist,0)


def ratio(j,x,prob,Px,Cx):

    eCx = map(lambda x: math.e**x if x != "deleted" else "deleted",Cx)
    ePx = map(lambda x: math.e**x if x != "deleted" else "deleted",Px)

    MijeCx = [coeff.val * eCx[coeff.cons] for coeff in  prob.c_var_i[j]]
    MijePx = [coeff.val * ePx[coeff.cons] for coeff in prob.p_var_i[j]]

    partialjPx = my_sum(MijePx)/my_sum(ePx)
    partialjCx = my_sum(MijeCx)/my_sum(eCx)

    return partialjPx/partialjCx


#to do
#more testing
#back_up
#instead of reevaluating the whole matrix just update the sums
#check time on deletes
#update round robin style
#bucket sort index by variable matrix by value
#on updates only add to sums approximatly
#

prob = create_problem('problem')
x = init_x(prob.num_vars)
prob.set_N(x)
prob.scale_coeffs()
prob.min_CX = min(eval_Cx(prob,x))
prob.max_Ci = find_max_Ci(prob)
prob.max_Pi = find_max_Pi(prob)
prob.num_del = 0
iterations = 0

while True:
    
    iterations = iterations + 1
    all_deleted = delete_cov_constraints(prob,x)
    if all_deleted: break
    j,inc_amount = find_incrament(prob,x)    
    x[j] = x[j] + inc_amount

    
    
print 'variables'
print x
print 'N'
print prob.N
print 'iterations'
print iterations












