#!/usr/bin/env python2.7

import random
import math
#import sys
#import pstats
#import cProfile

#Global variables

eps = 0.1

class Coeff:
    def __init__(self, value, index):
        self.value = value
        self.index = index
    removed = False

class Sampler:
    down_scale_factor = eps
    small_bound = int(1/eps)
    up_scale_bound = 536870912  # 2^30

    def __init__(self, n):


def reorder_up(ordered, index):
    while (index > 0
           and ordered[index].value > ordered[index-1].value):
        ordered[index], ordered[index-1] = ordered[index-1], ordered[index]
        index -= 1
    return ordered


def reorder_down(ordered, index):
    while (index < len(ordered) - 1
           and ordered[index].value < ordered[index+1].value):
            ordered[index], ordered[index+1] = ordered[index+1], ordered[index]
            index += 1
    return ordered


def solve(r, c, m, mt, m_linker, m_copy, mt_copy):

    global eps, down_scale_factor, small_bound, up_scale_bound

    deleted_cols = set()

    x = [0]*c
    xh = [0]*r
    y = [0]*r
    yh = [0]*c

    u = [t_max(row) for row in m]
    uh = [t_max(row) for row in mt]

    dist_item_max = int(up_scale_bound/(r)/max(u))-1
    down_scale_bound = int(dist_item_max/(1/eps))

    # p  = [ (1+eps)**yi for yi in y]
    p = [Sampler(down_scale_bound, index) for index in range(r)]

    # ph  = [ (1-eps)**yhj for yhj in yh]
    ph = [Sampler(dist_item_max, index) for index in range(c)]

    pXuh = map(lambda uhi, pi: Sampler(uhi*pi.value, pi.index), uh, p)
    phXu = map(lambda ui, phi: Sampler(ui*phi.value, phi.index), u, ph)

    ps = samp_sum(p)
    phs = samp_sum(ph)
    pXuhs = samp_sum(pXuh)
    phXus = samp_sum(phXu)

    p_ordered = sorted(p, cmp_r)
    ph_ordered = sorted(ph, cmp_r)
    pXuh_ordered = sorted(pXuh, cmp_r)
    phXu_ordered = sorted(phXu, cmp_r)

    underflows_p = [0]*c
    underflows_pXuh = [0]*c
    underflows_ph = [0]*r
    underflows_phXu = [0]*r

    iterations = 0
    N = 2*math.log(r*c)/eps**2

    while max(y) < N and min(yh) < N:
        iterations += 1
        ip, jp = random_pair(p_ordered, ph_ordered,
                             pXuh_ordered, phXu_ordered,
                             ps, phs, pXuhs, phXus)
        # print iterations

        delta = 1.0/(uh[ip] + u[jp])
        x[jp] += delta
        xh[ip] += delta
        z = random.random()

        for item in mt[jp]:
            if item.value*delta < z:
                break

            index = item.index
            y[index] += 1

            if underflows_p[index] <= 0:
                ps += int(round(p[index].value*eps))
                p[index].value += int(round(p[index].value*eps))
                reorder_up(p_ordered, index)
            else:
                underflows_p[index] -= 1
                if underflows_p[index] == 0:
                    p[index].value = small_bound
                    ps += small_bound

            if ps >= up_scale_bound:
                ps = down_scale_values(ps, p, underflows_p)

            if underflows_pXuh[index] <= 0:
                pXuhs += int(round(pXuh[index].value*eps))
                pXuh[index].value += int(round(pXuh[index].value*eps))
                reorder_up(pXuh_ordered, index)
            else:
                underflows_pXuh[index] -= 1
                if underflows_pXuh[index] == 0:
                    pXuh[index].value = small_bound
                    pXuhs += small_bound

            if pXuhs >= up_scale_bound:
                pXuhs = down_scale_values(pXuhs, pXuh, underflows_pXuh)

            if ps != samp_sum(p) or ps > up_scale_bound:
                print 'p overflowed'
                exit(1)

        for item in m[ip]:
            if item.removed:
                continue
            if item.value*delta < z:
                break
            index = item.index
            if index in deleted_cols:
                continue

            if yh[index] >= N:
                deleted_cols.add(index)
                phs -= ph[index].value
                phXus -= phXu[index].value
                ph[index].value = 0
                phXu[index].value = 0
                for m_index, row in enumerate(m_linker):
                    m_linker[m_index][index].removed = True
                for m_index, row in enumerate(m):
                    found = False
                    for item in row:
                        if item.removed:
                            continue
                        if u[m_index] == 0:
                            found = True
                            break
                        old = phXu[m_index].value
                        phXu[m_index].value = int(
                            round(phXu[m_index].value/u[m_index]*item.value))
                        phXus -= old-phXu[m_index].value
                        u[m_index] = item.value
                        found = True
                        break
                    if not found:
                        phXus -= phXu[m_index].value
                        phXu[m_index].value = 0
                        u[m_index] = 0

            yh[index] += 1

            if phs <= 1000000:  # fix magic
                print 'possible inaccuracies'
                exit(1)

            phs -= int(round(ph[index].value*eps))
            ph[index].value -= int(round(ph[index].value*eps))
            reorder_down(ph_ordered, index)

            if ph[index].value < small_bound:
                phs -= ph[index].value
                ph[index].value = 0
                underflows_ph[index] += 1

            if phs < int(up_scale_bound*(1-eps)):
                phs = up_scale_values(phs, ph, underflows_ph)

            phXus -= int(round(phXu[index].value*eps))
            phXu[index].value -= int(round(phXu[index].value*eps))
            reorder_down(phXu_ordered, index)

            if phXu[index].value < small_bound:
                phXus -= phXu[index].value
                phXu[index].value = 0
                underflows_phXu[index] += 1

            if phXus < int(up_scale_bound*(1-eps)):
                phXus = up_scale_values(phXus, phXu, underflows_phXu)

            if phs != samp_sum(ph) or phs > up_scale_bound:
                print 'ph overflowed'
                exit(1)

    print 'normalize for final solution'

    mix = [sum(map(lambda xe, me: xe*me[0], x, row)) for row in m_copy]
    mtjxh = [sum(map(lambda xe, me: xe*me[0], xh, row)) for row in mt_copy]

    #print x
    #print xh
    #print mix
    #print mtjxh
    #print max(mix)
    #print min(mtjxh)
    print [i.value for i in p_ordered]
    print [i.value for i in ph_ordered]
    primal = map(lambda element, m=max(mix): element/m, x)
    dual = map(lambda element, m=min(mtjxh): element/m, xh)

    print 'answer'
    print iterations, 'iterations'
    print 'primal solution:', primal, 'value:', sum(primal)
    print 'dual solution:', dual, 'value:', sum(dual)
    print 'primal cost is', (sum(primal)/sum(dual)), 'of dual'


def random_pair(p, ph, pXuh, phXu, ps, phs, pXuhs, phXus):
    prob = pXuhs*phs/(pXuhs*phs+ps*phXus)
    z = random.random()
    if prob >= z:
        return sample(pXuh, pXuhs), sample(ph, phs)
    else:
        return sample(p, ps), sample(phXu, phXus)


def sample(dist, dist_sum):
    s = 0
    z = random.uniform(0, dist_sum)
    for index, element in enumerate(dist):
        s = s + element.value
        if s > z:
            return element.index
    print dist_sum, samp_sum(dist)
    print 'find fail'


def up_scale_values(size, dist, underflows):
    global small_bound
    total_change = 0
    for index, item in enumerate(dist):
        if underflows[index] <= 0:
            total_change += int(round(item.value*eps))
            item.value += int(round(item.value*eps))
        else:
            underflows[index] -= 1
            if underflows[index] == 0:
                item.value = small_bound
                total_change += small_bound

        if item.value > up_scale_bound:
            print 'we have a problem in up scale values'
            exit(1)

    return size + total_change


def down_scale_values(size, dist, underflows):
    total_change = 0
    for index, item in enumerate(dist):
        total_change += int(round(item.value*down_scale_factor))
        item.value -= int(round(item.value * down_scale_factor))
        if item.value < small_bound:
            underflows[index] += 1
            total_change += dist[index].value
            item.value = 0

    return size - total_change


def cmp_r(x, y):
    return cmp(y.value, x.value)


def samp_sum(array):
    return sum(i.value for i in array)


def t_max(array):
    return max(i.value for i in array)


####################################

#### global  inputs

## use
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

##      m_r,m_c,val = line.split()
##      #m[int(m_r)].append(float(val))
##      m[int(m_r)][int(m_c)] = float(val)
##      m_copy[int(m_r)][int(m_c)] = float(val)
##
##      mt[int(m_c)][int(m_r)] = float(val)
##      mt_copy[int(m_c)][int(m_r)] = float(val)
##     #mt[int(m_c)].append(float(val))


def main():
    #  random example
    upper_value = 10
    lower_value = 1  # does not work if less than 1
    r = 10
    c = 10

    m = [[Coeff(0, i) for i in range(c)] for j in range(r)]
    mt = [[Coeff(0, i) for i in range(r)] for j in range(c)]

    m_copy = [[[0, i] for i in range(c)] for j in range(r)]
    mt_copy = [[[0, i] for i in range(r)] for j in range(c)]

    m_linker = [row[:] for row in m]

    for i in range(r):
        for j in range(c):
            if(random.random() > 0.5):
                my_rand = random.randint(lower_value, upper_value)
                m[i][j].value = my_rand
                m_copy[i][j] = [my_rand, j]
                mt[j][i].value = my_rand
                mt_copy[j][i] = [my_rand, i]

    m = [sorted(row, cmp_r) for row in m]
    mt = [sorted(row, cmp_r) for row in mt]

    # cProfile.run('solve(p, ph, pXuh, phXu, ps, phs, pXuhs, phXus,x,xh,y,yh,N,c,r )')
    solve(r, c, m, mt, m_linker, m_copy, mt_copy)

main()
