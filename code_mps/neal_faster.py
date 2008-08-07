import random
import math,sys
#import cProfile
import pstats

class Sampler:
	def __init__(self, value,index):
		self.value = value
		self.index =  index

	value = 0

class Coeff:
	def __init__(self, value,index):
		self.value = value
		self.index =  index

	value = 0
	removed = False


def up_date_order_up(order, index):

	while(1):
		
		if index == 0:
			break
		if order[index].value>order[index-1].value:
			order[index],order[index-1] = order[index-1], order[index]
			index = index - 1
			continue
		else:
			break
	return order
	

def up_date_order_down(order,index):

	while(1):
		if index == len(order)-1:
			break
		if order[index].value<order[index+1].value:
			order[index],order[index+1] = order[index+1], order[index]
			index = index + 1
			continue
		else:
			break
	return order

def solve(p, ph, pXuh, phXu, ps, phs, pXuhs, phXus,x,xh,y,yh,N,c,r,over_flow_p,over_flow_x ,over_flow_ph,over_flow_phx,j,p_ordered, ph_ordered, pXuh_ordered, phXu_ordered):

	stop = 0
	scales = 0
	while max(y)< N and min(yh)< N:
		stop = stop +1
		ip,jp = random_pair(p_ordered, ph_ordered, pXuh_ordered, phXu_ordered, ps, phs, pXuhs, phXus)
		print stop
		
		delta = 1.0/(uh[ip] + u[jp])
		x[jp] = x[jp] +delta
		
		xh[ip] = xh[ip] +delta
		z = random.random()

		for index,item in enumerate(mt[jp]):

			if item.value*delta >= z:
				index = item.index
				y[index] = y[index] + 1
				if over_flow_p[index] > 0:
					over_flow_p[index] =  over_flow_p[index] - 1
					if over_flow_p[index] == 0:
						p[index].value = small_bound
						ps = ps + small_bound
				else:	
					ps  = ps + int(round(p[index].value*eps))
					p[index].value = p[index].value + int(round(p[index].value*eps))
					up_date_order_up(p_ordered, index)
				
				if ps >= up_scale_bound:
					ps,p,over_flow_p = down_scale_values(ps,p,over_flow_p)
				
				if over_flow_x[index] > 0:

					over_flow_x[index] =  over_flow_x[index] - 1
					if over_flow_x[index] == 0:
						pXuh[index].value = small_bound
						pXuhs = pXuhs + small_bound
				else:
				
					pXuhs  = pXuhs+int(round(pXuh[index].value*eps))
					pXuh[index].value = pXuh[index].value + int(round(pXuh[index].value*eps))
					up_date_order_up(pXuh_ordered, index)
				
				if pXuhs >= up_scale_bound:
					pXuhs,pXuh,over_flow_x = down_scale_values(pXuhs,pXuh,over_flow_x)

				if ps != samp_sum(p) or  ps > up_scale_bound:
					print 'p over flowed'
					exit(1)

			else: break

		for  my_index,item in enumerate(m[ip]):

			if item.removed:
				continue
			
			if item.value*delta >= z:
				index = item.index
				if j[index] == -1:
					continue

				#check if we really need to update all the us
				if yh[index] >=  N:
					j[index] = -1
					phs =  phs - ph[index].value
					phXus =  phXus - phXu[index].value
					ph[index].value=0
					phXu[index].value=0
					for m_index,row in enumerate(m_linker):
						m_linker[m_index][index].removed = True
					for m_index,row in enumerate(m):
						found = False
						for item in row:
							if not item.removed:
								if  u[m_index] == 0:
									found = True
									break
								old  =phXu[m_index].value
								phXu[m_index].value = int(round(phXu[m_index].value/u[m_index]*item.value))
								phXus = phXus - (old-phXu[m_index].value)
								u[m_index] =  item.value
								found = True
								break
						if not found:
							phXus = phXus - phXu[m_index].value
							phXu[m_index].value = 0
							u[m_index] = 0
						
				yh[index] = yh[index] + 1
				
				if phs <= 1000000: #fix magic
					print 'possible inaccuracies'
					exit(1)
					
				phs  = phs - int(round(ph[index].value*eps))
				ph[index].value = ph[index].value - int(round(ph[index].value*eps))
				up_date_order_down(ph_ordered, index)
				
				if ph[index].value < small_bound:
					phs = phs - ph[index].value
					ph[index].value = 0
					over_flow_ph[index] =  over_flow_ph[index] + 1
				
				if phs <  int(up_scale_bound*(1-eps)):
					phs,ph, over_flow_ph = up_scale_values(phs,ph,over_flow_ph)
				
				phXus  = phXus- int(round(phXu[index].value*eps))
				phXu[index].value = phXu[index].value - int(round(phXu[index].value*eps))
				up_date_order_down(phXu_ordered, index)
				
				if phXu[index].value < small_bound:
					phXus = phXus - phXu[index].value
					phXu[index].value = 0
					over_flow_phx[index] =  over_flow_phx[index] + 1
				
				if phXus < int(up_scale_bound*(1-eps)):
					phXus,phXu,over_flow_phx = up_scale_values(phXus,phXu,over_flow_phx)

				if phs != samp_sum(ph) or phs >up_scale_bound:
					print 'ph over flowed'
					exit(1)
				
			else:
				break 

	print 'normalize for final solution'

	mix = [sum(map(lambda xe,me: xe*me[0], x,row)) for row in m_copy ]
	mtjxh = [ sum(map(lambda xe,me: xe*me[0], xh,row)) for row in mt_copy]

	#print x
	#print xh
	#print mix
	#print mtjxh
	#print max(mix)
	#print min(mtjxh)
	print [i.value for i in p_ordered]
	print  [i.value for i in ph_ordered]
	primal,duel = map(lambda element: element/max(mix), x), map(lambda element: element/min(mtjxh), xh)

	print 'answer'
	print stop, ' iterations requiered'
	print 'primal solution: ',primal ,'value: ',sum(primal)
	print 'deul solution: ',duel,  'value: ', sum(duel) 
	print 'primal is  at least ', (sum(primal)/sum(duel)),' of opt'


def random_pair(p, ph, pXuh, phXu, ps, phs, pXuhs, phXus):
	prob = pXuhs*phs/(pXuhs*phs+ps*phXus)
	z = random.random()
	if prob >= z:
		return sample(pXuh, pXuhs), sample(ph,phs)
	else:
		return sample(p,ps), sample(phXu,phXus)


def sample(dist, dist_sum):

	s = 0
	z = random.uniform(0,dist_sum)
	for index, element in enumerate(dist):
		s = s + element.value
		if s > z:
			return element.index
	print dist_sum,samp_sum(dist)
	print 'find fail'


def up_scale_values(size,distro,over_flow):

	total_change = 0
	for index,item in enumerate(distro):
		
		if over_flow[index] > 0:
			over_flow[index] = over_flow[index] -1
			if over_flow[index] == 0:
				distro[index].value = small_bound
				total_change = total_change + small_bound
		else:
			total_change = total_change + int(round(item.value*eps))
			distro[index].value = item.value + int(round(item.value*eps))
			
		if distro[index].value > up_scale_bound:
			print 'we have a problem in up scale values'
			exit(1)
	
	return size + total_change, distro,over_flow

def down_scale_values(size,distro,over_flow):

	total_change = 0
	for index,item in enumerate(distro):
		total_change = int(round(item.value*down_scale_factor)) + total_change
		distro[index].value = item.value - int(round(item.value *down_scale_factor))
		if distro[index].value < small_bound:
			over_flow[index] = over_flow[index]+1
			total_change =  total_change + distro[index].value
			distro[index].value = 0
			
	return size - total_change, distro,over_flow

def samp_sum(array):
	total = 0
	for item in array:
		total = item.value + total
	return total


def cmp_r(x,y):
	if y.value >x.value: return 1
	elif x.value>y.value:return -1
	else: return 0


def samp_cmp(x,y):

	if y.value > x.value: return 1
	elif x.value>y.value:return -1
	else: return 0

def t_max(array):
	my_max = 0
	for i in array:
		if i.value>my_max: my_max = i.value
	return my_max

####################################
#Global variables

eps = 0.1
down_scale_factor = eps
small_bound = int(1/eps)
up_scale_bound =   536870912 # 2^30


#### global  inputs

##use 
## file_name = 'fastpc/code_mps/test/test_lp_input_2' #file input does not work
## my_file = open(file_name)

## r,c,non_zero= my_file.next().split()
## print r
## print c
## print non_zero
## r = int(r)
## c = int(c)




## m = [ [0 for i in range(c)] for j in range(r)]
## m_copy = [ [0 for i in range(c)] for j in range(r)]
## mt = [ [0 for i in range(r)] for j in range(c)]
## mt_copy = [ [0 for i in range(r)] for j in range(c)]

## for line in my_file:

## 	m_r,m_c,val = line.split()
## 	#m[int(m_r)].append(float(val))
## 	m[int(m_r)][int(m_c)] = float(val)
## 	m_copy[int(m_r)][int(m_c)] = float(val)
	
## 	mt[int(m_c)][int(m_r)] = float(val)
## 	mt_copy[int(m_c)][int(m_r)] = float(val)
##         #mt[int(m_c)].append(float(val))


#random example
upper_value = 10
lower_value = 1 #does not work if less than 1
r = 10
c = 10

m = [ [Coeff(0,i) for i in range(c)] for j in range(r)]
mt = [ [Coeff(0,i) for i in range(r)] for j in range(c)]

m_copy = [ [[0,i] for i in range(c)] for j in range(r)]
mt_copy = [ [[0,i] for i in range(r)] for j in range(c)]

m_linker =  [ [item for item in row] for row in m]
mt_linker =  [ [item for item in row] for row in mt]

for i in range(r) :
	for j in range(c):
		if(random.random() > 0.5):
			my_rand = random.randint(lower_value,upper_value) 
			m[i][j].value = my_rand
			m_copy[i][j] =  [my_rand,j]
			mt[j][i].value = my_rand
			mt_copy[j][i] = [my_rand,i]

print 'start sort'
m = map(lambda row: sorted(row,cmp_r),m)
mt = map(lambda row: sorted(row,cmp_r),mt)
print 'end sort'

print 'setting up variables'

j = [0]*c

over_flow_p = [0]*c
over_flow_x = [0]*c
over_flow_ph = [0]*r
over_flow_phx = [0]*r

x=[0]*c
xh=[0]*r
y=[0]*r
yh=[0]*c

u = map(lambda row: t_max(row) ,m)
uh = map(lambda row: t_max(row),mt)

#distro_item_max =  int(up_scale_bound/(r))
distro_item_max = int(up_scale_bound/(r)/max(u))-1

down_scale_bound = int(distro_item_max/(1/eps))

#p  = [ (1+eps)**yi for yi in y]
p = []
p_ordered = []
p = [Sampler(down_scale_bound,index) for index,i in enumerate(y)]
p_ordered = [i for i in p]

#ph  = [ (1-eps)**yhj for yhj in yh]
ph = []
ph_ordered = []
ph = [Sampler(distro_item_max,index) for index, i in enumerate(yh)]
ph_ordered = [i for i in ph]
	
N = 2*math.log(r*c)/eps**2

pXuh = map(lambda uhi,pi: Sampler(uhi*pi.value,pi.index),uh,p)
pXuh_ordered = map(lambda item: item,pXuh)

phXu = map(lambda ui,phi: Sampler(ui*phi.value,phi.index),u,ph)
phXu_ordered = map(lambda item: item,phXu)

ps = samp_sum(p) 
phs = samp_sum(ph)
pXuhs = samp_sum(pXuh)
phXus = samp_sum(phXu)

p_ordered.sort(samp_cmp)
ph_ordered.sort(samp_cmp)
pXuh_ordered.sort(samp_cmp)
phXu_ordered.sort(samp_cmp)


print 'solve'
#cProfile.run('solve(p, ph, pXuh, phXu, ps, phs, pXuhs, phXus,x,xh,y,yh,N,c,r )')
solve(p, ph, pXuh, phXu, ps, phs, pXuhs, phXus,x,xh,y,yh,N,c,r ,over_flow_p,over_flow_x,over_flow_ph,over_flow_phx,j, p_ordered, ph_ordered, pXuh_ordered, phXu_ordered)

