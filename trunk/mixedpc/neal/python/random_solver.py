#!/usr/bin/env python3.4

from math import log, ceil
from sys import _getframe
from collections import defaultdict
import random

from utilities import *
from matrix import Matrix


def solve(P, C, eps):
    P.done_adding_entries()
    C.done_adding_entries()
    assert 0 < eps <= 0.1
    n = max(P.n_cols, C.n_cols)
    x = [0]*n
    m_p = P.n_rows
    m_c = C.n_rows
    m = m_p + m_c
    U = log(m)/eps**2
    if m_c == 0:
        return x
    P_hat = [0]*m_p
    exp_P_hat = [1.0]*m_p
    C_hat = [0]*m_c
    exp_C_hat = [1.0]*m_c
    factor = (1+eps)**2 / (1-eps)

    outer_iterations = 0
    inner_steps = 0
    wasted_steps = 0
    ratio_distribution = defaultdict(lambda: 0)

    Pj_dot_p_hat_fn = lambda j: P.col_dot_vector(j, exp_P_hat)
    Cj_dot_c_hat_fn = lambda j: C.col_dot_vector(j, exp_C_hat)

    sum_exp_P_hat = sum(exp_P_hat)
    sum_exp_C_hat = sum(exp_C_hat)

    def report():
        nonlocal inner_steps, wasted_steps, outer_iterations
        nonlocal sum_exp_P_hat, sum_exp_C_hat
        print(dump("outer_iterations",
                   "inner_steps; wasted_steps; wasted_steps/inner_steps",
                   "sum_exp_C_hat, sum_exp_P_hat"))
        # print("ratio distribution:")
        # for (e,count) in sorted(ratio_distribution.items()):
        #     print(e, ":", count)

    while True:
        outer_iterations += 1
        changes_to_x = 0
        for j in range(n):
            Pj_dot_p_hat = Pj_dot_p_hat_fn(j)
            Cj_dot_c_hat = Cj_dot_c_hat_fn(j)

            lambda_j = lambda: Pj_dot_p_hat / Cj_dot_c_hat \
                if Cj_dot_c_hat > 0 \
                else 10 * sum_exp_P_hat / sum_exp_C_hat

            for e in P.col_entries(j):
                wasted_steps += 1
            for e in C.col_entries(j):
                wasted_steps += 1

            inner_loops = 0

            def random_update_P(dx, threshold):
                nonlocal Pj_dot_p_hat, inner_loops, inner_steps, sum_exp_P_hat
                e = None
                for e in P.col_entries(j):
                    inner_steps += 1
                    if dx*e.value < threshold:
                        break
                    P_hat[e.row] += 1
                    prev_value = exp_P_hat[e.row]
                    exp_P_hat[e.row] *= (1+eps)
                    Pj_dot_p_hat += (exp_P_hat[e.row] - prev_value)*e.value
                    sum_exp_P_hat += exp_P_hat[e.row] - prev_value
                P.col_compact(j, e)

            def random_update_C(dx, threshold):
                nonlocal Cj_dot_c_hat, inner_loops, inner_steps, sum_exp_C_hat
                e = None
                for e in C.col_entries(j):
                    inner_steps += 1
                    if dx*e.value < threshold:
                        break
                    C_hat[e.row] += 1
                    prev_value = exp_C_hat[e.row]
                    if C_hat[e.row] < U:
                        exp_C_hat[e.row] *= (1-eps)
                    else:
                        exp_C_hat[e.row] = 0
                        C.remove_entries_in_row(e.row)
                    Cj_dot_c_hat += (exp_C_hat[e.row] - prev_value)*e.value
                    sum_exp_C_hat += exp_C_hat[e.row] - prev_value
                C.col_compact(j, e)

            # ratio_distribution[ceil(log(lambda_j() 
            #                             / (sum_exp_P_hat/sum_exp_C_hat),
            #                             1+eps))] += 1

            while lambda_j() <= factor * sum_exp_P_hat / sum_exp_C_hat:
                changes_to_x += 1
                inner_loops += 1
                z = 1 / max(P.col_max(j), C.col_max(j))
                threshold = random.random()
                x[j] += z
                random_update_P(z, threshold)
                random_update_C(z, threshold)
                if C.n_non_empty_rows == 0:
                    report()
                    return [z/U for z in x]
            if C.n_non_empty_rows == 0:
                report()
                return [z/U for z in x]

        if True or changes_to_x:
            # return early if (1+eps)-feasible
            maxPx = max(P.times_vector(x))
            minCx = min(C.times_vector(x))
            if outer_iterations % 1000 == 0:
                print("outer iterations:", outer_iterations, 
                      "inner steps:", inner_steps, end="")
                if minCx > 0:
                    print(", eff eps: %.3f" % (maxPx/minCx-1))
                else:
                    print(", eff eps: inf")
            if maxPx < (1+eps)*minCx:
                report()
                return [z/minCx for z in x]

            if all(
                    Pj_dot_p_hat_fn(j) * sum_exp_C_hat
                    >
                    sum_exp_P_hat * Cj_dot_c_hat_fn(j)
                    for j in range(n)
            ):
                report()
                print("infeasible")
                return None

            def close(a, b):
                return -0.01 < (a-b)/a < 0.01

            assert close(sum(exp_C_hat), sum_exp_C_hat), dump(
                "outer_iterations; inner_steps; U; x; sum_exp_C_hat; exp_C_hat; C_hat")
            assert close(sum(exp_P_hat), sum_exp_P_hat), dump(
                "outer_iterations; inner_steps; U; x; sum_exp_P_hat; exp_P_hat; P_hat")

            sum_exp_P_hat = sum(exp_P_hat)
            sum_exp_C_hat = sum(exp_C_hat)

if __name__ == '__main__':
    import sys

    n = 15
    epsilon = 0.03

    if len(sys.argv) > 1:
        epsilon = float(sys.argv[1])
    if len(sys.argv) > 2:
        n = int(sys.argv[2])

    random.seed(3141592)
    P, C, x = random_P_C_x(n)
    y = solve(P, C, epsilon)
    if y is not None:
        Py = P.times_vector(y)
        Cy = C.times_vector(y)
        print("x:", str_vector(x))
        print("y:", str_vector(y))
        print("min Cy:", min(Cy))
        print("max Py:", max(Py))
        print("eff. eps.:", max(Py)/min(Cy) - 1.0)
