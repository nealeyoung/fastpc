#!/usr/bin/env python3.4

from math import log, ceil, log1p
from collections import defaultdict
import random

from utilities import *


def solve(P, C, eps, start=None):
    P.done_adding_entries()
    C.done_adding_entries()
    assert 0 < eps <= 0.1
    n = max(P.n_cols, C.n_cols)
    m_p = P.n_rows
    m_c = C.n_rows
    if m_c == 0:
        return [0]*n
    m = m_p + m_c
    U = 2*log(m)/eps**2
    x = [0]*n

    problem_size = len(P.entries) + len(C.entries)

    print(dump("problem_size"))

    for i in range(m_p):
        for k, e in enumerate(P.row_original_entries(i)):
            e.index_in_row = k

    for i in range(m_c):
        for k, e in enumerate(C.row_original_entries(i)):
            e.index_in_row = k

    class cells:
        Px = [cell_sum((cell(lambda e=e: e.value * x[e.col],
                             name="Px[%d][%d]" % (i, e.col))
                       for e in P.row_original_entries(i)),
                       name="Px[%d]" % i)
              for i in range(m_p)]

        Cx = [cell_sum((cell(lambda e=e: e.value * x[e.col],
                             name="Cx[%d][%d]" % (i, e.col))
                       for e in C.row_original_entries(i)),
                       name="Cx[%d]" % i)
              for i in range(m_c)]

        max_Px = cell(lambda Px=Px: max(c.value for c in Px), name="max_Px")
        min_Cx = cell(lambda Cx=Cx: min(c.value for c in Cx), name="min_Cx")

        @staticmethod
        def maybe_remove_row(i):
            if cells.Cx[i].value < U:
                return
            print("remove row", i)
            C.remove_entries_in_row(i)
            cells.Cx[i].destroy()

        for i in range(m_c):
            Cx[i].add_hook(lambda *args, _i=i: cells.maybe_remove_row(_i))

        alpha = -log1p(-eps)/log1p(eps)
        shift = 0

        # beware comprehensions in class definitions
        # http://stackoverflow.com/questions/13905741/accessing-class-variables-from-a-list-comprehension-in-the-class-definition

        @staticmethod
        def reset():
            cells.shift = min(C.times_vector(x))  # NOT WORKING ??

            for cs in cells.Px:
                cs.reset()
            for cs in cells.Cx:
                cs.reset()
            cells.p_hat.reset()
            cells.c_hat.reset()

            cells.min_Cx.reset()
            cells.max_Px.reset()

    cells.p_hat = cell_sum((
        cell((lambda i=i:
              (1+eps)**(cells.Px[i].value - cells.alpha*cells.shift)),
             cells.Px[i],
             name="p_hat[%d]" % i)
        for i in range(m_p)),
        name='p_hat')

    cells.c_hat = cell_sum((
        cell((lambda i=i: (1-eps)**(cells.Cx[i].value - cells.shift)
              if cells.Cx[i].value < U else 0),
             cells.Cx[i],
             name="c_hat[%d]" % i)
        for i in range(m_c)),
        name='c_hat')

    def potential():
        return cells.p_hat.value * cells.c_hat.value / (m_p * m_c)

    Pj_dot_p_hat_fn = lambda j: P.col_dot_vector(j, cells.p_hat)
    Cj_dot_c_hat_fn = lambda j: C.col_dot_vector(j, cells.c_hat)

    factor = (1+eps)**2 / (1-eps)

    class stats:
        outer_iterations = 0
        inner_steps = 0
        inner_steps_last_check = 0
        inner_steps_last_outer = 0
        wasted_steps = 0
        ratio_distribution = defaultdict(lambda: 0)

    def report():
        nonlocal stats, cells, potential
        print(dump("stats.outer_iterations",
                   "stats.inner_steps; stats.wasted_steps",
                   "stats.wasted_steps/(1+stats.inner_steps)",
                   "cells.c_hat.value, cells.p_hat.value",
                   "potential()"))
        # print("ratio distribution:")
        # for (e,count) in sorted(ratio_distribution.items()):
        #     print(e, ":", count)

    # def rollforward(limit=-1):
    #     while limit != 0:
    #         limit -= 1
    #         x[:] = [_*(1+eps) for _ in x]
    #         cells.reset()
    #         min_Cx = cells.min_Cx.value
    #         max_Px = cells.max_Px.value
    #         if potential() > 1 or cells.min_Cx.value > 0.99*U:
    #             break
    #         print(
    #             "++  eff eps=", '%.3g' % (max_Px/min_Cx-1),
    #             ", potential=", '%.3g' % potential(),
    #             ", p_hat=", "%.3g" % cells.p_hat.value,
    #             ", c_hat=", "%.3g" % cells.c_hat.value,
    #             ", max_Px-alpha*min_Cx=", "%.3g" % (
    #                 max_Px - cells.alpha*min_Cx))

    # # beware, this interacts badly with removed rows
    # def rollback(limit=-1):
    #     while limit != 0:
    #         limit -= 1
    #         x[:] = [_/(1+eps) for _ in x]
    #         cells.reset()
    #         if potential() < 0.3 or cells.min_Cx.value < 1:
    #             break
    #         min_Cx = cells.min_Cx.value
    #         max_Px = cells.max_Px.value
    #         print(
    #             "--  eff eps=", '%.3g' % (max_Px/min_Cx-1),
    #             ", potential=", '%.3g' % potential(),
    #             ", p_hat=", "%.3g" % cells.p_hat.value,
    #             ", c_hat=", "%.3g" % cells.c_hat.value,
    #             ", max_Px-alpha*min_Cx=", "%.3g" % (
    #                 max_Px - cells.alpha*min_Cx))

    def check_done():
        nonlocal stats, cells, m_p, m_c

        # return early if (1+eps)-feasible
        min_Cx = cells.min_Cx.value
        max_Px = cells.max_Px.value

        eff_eps = max_Px/min_Cx-1 if min_Cx > 0 else 1
        print("outer_iterations=", stats.outer_iterations,
              ", eff eps=", '%.3g' % eff_eps,
              ", potential=", '%.3g' % potential(),
              ", min_Cx=", "%.3g" % min_Cx,
              ", max_Px=", "%.3g" % max_Px,
              ", max_Px-alpha*shift=", "%.3g" % (
                  max_Px - cells.alpha*cells.shift))
        if eff_eps <= eps:
            report()
            return [_/min_Cx for _ in x]

        # check infeasibility
        assert cells.c_hat.value > 0 and cells.p_hat.value > 0
        assert -0.01 < sum(c for c in cells.c_hat)/cells.c_hat.value - 1 < 0.01
        assert -0.01 < sum(c for c in cells.p_hat)/cells.p_hat.value - 1 < 0.01

        if all(
                Pj_dot_p_hat_fn(j) / cells.p_hat.value
                > Cj_dot_c_hat_fn(j) / cells.c_hat.value
                for j in range(n)):

            for j in range(n):
                print(dump(
                    "j, Pj_dot_p_hat_fn(j)/cells.p_hat.value, Cj_dot_c_hat_fn(j)/cells.c_hat.value"
                    ))
            return 'infeasible'
        return None

    # if start is not None:
    #     x[:] = [_*U/2 for _ in start]
    #     cells.reset()

    def time_to_check():
        return stats.inner_steps-stats.inner_steps_last_check > 30*problem_size

    while True:
        stats.outer_iterations += 1
        stats.inner_steps_last_outer = stats.inner_steps

        if stats.outer_iterations % 100 == 0:
            print(dump("stats.outer_iterations, stats.inner_steps"))

        for j in range(n):

            Pj_dot_p_hat = cell_sum((
                cell(lambda e=e: e.value * cells.p_hat[e.row],
                     cells.p_hat.deps[e.row])
                for e in P.col_entries(j)),
                name="Pj_dot_p_hat")

            Cj_dot_c_hat = cell_sum((
                cell(lambda e=e: e.value * cells.c_hat[e.row],
                     cells.c_hat.deps[e.row])
                for e in C.col_entries(j)),
                name="Cj_dot_c_hat")

            for e in C.col_entries(j):
                e.last_xj = x[j]

            for e in P.col_entries(j):
                e.last_xj = x[j]

            stats.wasted_steps \
                += len(Pj_dot_p_hat.deps) + len(Cj_dot_c_hat.deps)

            j_ok = (lambda:
                    Pj_dot_p_hat.value * cells.c_hat.value
                    <
                    factor * Cj_dot_c_hat.value * cells.p_hat.value)

            stats.inner_loops = 0

            # ratio_distribution[ceil(log(lambda_j() / lambda_0, 1+eps))] += 1

            def update(M, Mx, j, final=False):
                e = None
                rows = []
                for e in M.col_entries(j):
                    stats.inner_steps += 1
                    c = Mx[e.row].deps[e.index_in_row]
                    dxj = x[j] - e.last_xj

                    if e.top * dxj < 0.5 and not final:
                        break
                    delta = c.fn() - c.value
                    assert (final or 0.25 <= delta) and delta <= 2.1, \
                        dump("delta; j; final; e.row; e.value; c; c.deps")
                    c.update()
                    e.last_xj = x[j]
                M.col_compact(j, e)

            while j_ok():
                stats.inner_loops += 1
                dxj = 1 / max(P.col_max(j), C.col_max(j))
                x[j] += dxj

                update(P, cells.Px, j, final=False)
                update(C, cells.Cx, j, final=False)

                if C.n_non_empty_rows == 0:
                    break

                if time_to_check():
                    break

            if stats.inner_loops:
                update(P, cells.Px, j, final=True)
                update(C, cells.Cx, j, final=True)

            Pj_dot_p_hat.destroy()
            Cj_dot_c_hat.destroy()

            if C.n_non_empty_rows == 0:
                report()
                return [_/U for _ in x]

            if time_to_check():
                break

        if time_to_check() \
           or stats.inner_steps == stats.inner_steps_last_outer:
            stats.inner_steps_last_check = stats.inner_steps
            cells.reset()
            chk = check_done()
            if chk is not None:
                return chk


# def wrap(P, C, eps):
#     assert 0 < eps < 0.1
#     eps2 = eps
#     while eps2 < 0.05:
#         eps2 *= 1.2

#     x = None
#     while True:
#         print("#### wrap epsilon= %.g" % eps2)
#         x = solve(P, C, eps2, start=x)
#         if x is not None:
#             Px = P.times_vector(x)
#             Cx = C.times_vector(x)
#             eff_eps = max(Px)/min(Cx) - 1.0
#             print("x:", str_vector(x))
#             print("min Cx:", min(Cx))
#             print("max Px:", max(Px))
#             print("eff. eps.:", eff_eps)

#         if x is None or eps2 <= eps:
#             return x
#         eps2 = eps2/1.2
#         P.restore()
#         C.restore()

if __name__ == '__main__':
    import sys

    n = 15
    epsilon = 0.03

    if len(sys.argv) > 1:
        epsilon = float(sys.argv[1])
    if len(sys.argv) > 2:
        n = int(sys.argv[2])

    random.seed(3141592)
    P, C, y = random_P_C_x(n)
    x = solve(P, C, epsilon)
    if x is not None:
        if x == 'infeasible':
            if 0 < max(P.times_vector(y)) <= min(C.times_vector(y)):
                print("error: solve returned infeasible for feasible instance")
                print(dump("max(P.times_vector(y)), min(C.times_vector(y))"))
            else:
                print(x)
        else:
            minCx = min(C.times_vector(x))
            maxPx = max(P.times_vector(x))
            print("eff eps: %.3f" % (maxPx/minCx - 1))
            print("y:", str_vector(y))
            print("x:", str_vector(x))
