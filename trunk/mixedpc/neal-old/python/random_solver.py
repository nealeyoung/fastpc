#!/usr/bin/env python3.4

from math import log, ceil
from sys import _getframe
from collections import defaultdict
import random


class Entry:
    def __init__(self, row, col, value):
        self.row = row
        self.col = col
        self.value = value
        self.top = 2**ceil(log(value, 2))
        self.removed = False


class Matrix:
    def __init__(self):
        self.entries = []
        self.done_adding = False

    def add_entry(self, row, col, value):
        assert row >= 0 and col >= 0 and value >= 0
        if value > 0:
            self.entries.append(Entry(row, col, value))

    def done_adding_entries(self):
        if self.done_adding:
            return
        self.done_adding = True
        self.n_rows = 1 + max(e.row for e in self.entries)
        self.n_cols = 1 + max(e.col for e in self.entries)
        self.cols = [[] for i in range(self.n_cols)]
        self.rows = [[] for i in range(self.n_rows)]
        for e in self.entries:
            self.cols[e.col].append(e)
            self.rows[e.row].append(e)
        self.cols = [sorted(col, key=lambda e: e.value)
                     for col in self.cols]
        self.n_non_empty_rows = len(tuple(0 for r in self.rows if len(r)))

    def col_entries(self, j):
        if not (0 <= j < self.n_cols):
            return
        for e in reversed(self.cols[j]):
            if not e.removed:
                yield e

    def row_original_entries(self, i):
        if not (0 <= i < self.n_rows):
            return
        for e in self.rows[i]:
            yield e

    def row_dot_vector(self, i, x):
        return sum(x[e.col]*e.value for e in self.row_original_entries(i))

    def col_dot_vector(self, j, x):
        return sum(x[e.row]*e.value for e in self.col_entries(j))

    def times_vector(self, x):
        return [self.row_dot_vector(i, x) for i in range(self.n_rows)]

    def col_max(self, j):
        if not (0 <= j < self.n_cols and self.cols[j]):
            return 0
        assert not self.cols[j][-1].removed
        return self.cols[j][-1].value

    def col_compact(self, j, start_entry=None):
        col = self.cols[j]
        start = 0
        if start_entry is not None:
            try:
                start = next(i for i in reversed(range(len(col)))
                             if col[i] == start_entry)
            except StopIteration:
                pass
        col[start:] = [e1 for e1 in col[start:] if not e1.removed]

    def remove_entries_in_row(self, i):
        row = self.rows[i]
        if row:
            for e in row:
                if not e.removed:
                    e.removed = True
                    while self.cols[e.col] and self.cols[e.col][-1].removed:
                        self.cols[e.col].pop()
            # self.rows[i] = []
            self.n_non_empty_rows -= 1


def dump(*exprs):
    callers_locals = dict(_getframe(1).f_locals)
    exprs = [e.strip() for e1 in exprs for e in e1.split(";")]
    values = [eval(e, globals(), callers_locals) for e in exprs]
    return "\n".join("%s = %s" % z for z in zip(exprs, values))


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
    lambda_0 = m_p / m_c
    P_hat = [0]*m_p
    exp_P_hat = [1.0]*m_p
    C_hat = [0]*m_c
    exp_C_hat = [1.0]*m_c
    factor = (1+eps)**2 / (1-eps)

    outer_iterations = 0
    inner_steps = 0
    wasted_steps = 0
    ratio_distribution = defaultdict(lambda: 0)

    def report():
        nonlocal inner_steps, wasted_steps
        print(dump("inner_steps, wasted_steps, wasted_steps/inner_steps"))
        # print("ratio distribution:")
        # for (e,count) in sorted(ratio_distribution.items()):
        #     print(e, ":", count)

    Pj_dot_p_hat_fn = lambda j: P.col_dot_vector(j, exp_P_hat)
    Cj_dot_c_hat_fn = lambda j: C.col_dot_vector(j, exp_C_hat)

    while True:
        outer_iterations += 1
        changes_to_x = 0
        for j in range(n):
            Pj_dot_p_hat = Pj_dot_p_hat_fn(j)
            Cj_dot_c_hat = Cj_dot_c_hat_fn(j)
            lambda_j = lambda: Pj_dot_p_hat / Cj_dot_c_hat \
                if Cj_dot_c_hat > 0 else 10*lambda_0

            for e in P.col_entries(j):
                e.p_last_xj = x[j]
                wasted_steps += 1
            for e in C.col_entries(j):
                e.c_last_xj = x[j]
                wasted_steps += 1

            inner_loops = 0

            def update_P(final=False):
                nonlocal Pj_dot_p_hat, inner_loops, z, inner_steps
                e = None
                for e in P.col_entries(j):
                    inner_steps += 1
                    dx = x[j] - e.p_last_xj
                    if (not final) and dx*e.top < 0.5:
                        break
                    if dx == 0:
                        continue
                    e.p_last_xj = x[j]
                    assert (final or 0.25 <= dx * e.value) \
                        and dx * e.value <= 1.0001
                    P_hat[e.row] += dx * e.value
                    prev_value = exp_P_hat[e.row]
                    exp_P_hat[e.row] *= (1+eps)**(dx * e.value)
                    Pj_dot_p_hat += exp_P_hat[e.row]*e.value - prev_value
                P.col_compact(j, e)

            def update_C(final=False):
                nonlocal Cj_dot_c_hat, inner_loops, z, inner_steps
                e = None
                for e in C.col_entries(j):
                    inner_steps += 1
                    dx = x[j] - e.c_last_xj
                    if (not final) and dx*e.top < 0.5:
                        break
                    e.c_last_xj = x[j]
                    assert (final or 0.25 <= dx * e.value) \
                        and dx * e.value <= 1.0001
                    C_hat[e.row] += dx * e.value
                    prev_value = exp_C_hat[e.row]
                    if C_hat[e.row] < U:
                        exp_C_hat[e.row] *= (1-eps)**(dx * e.value)
                    else:
                        exp_C_hat[e.row] = 0
                        C.remove_entries_in_row(e.row)
                    Cj_dot_c_hat += exp_C_hat[e.row]*e.value - prev_value
                C.col_compact(j, e)

            ratio_distribution[ceil(log(lambda_j() / lambda_0, 1+eps))] += 1

            while lambda_j() <= factor * lambda_0:
                changes_to_x += 1
                inner_loops += 1
                z = 0.5 / max(P.col_max(j), C.col_max(j))
                x[j] += z
                update_P()
                update_C()
                if C.n_non_empty_rows == 0:
                    report()
                    return [z/U for z in x]
            if inner_loops:
                update_P(final=True)
                update_C(final=True)
                if C.n_non_empty_rows == 0:
                    report()
                    return [z/U for z in x]

        if changes_to_x:
            # return early if (1+eps)-feasible
            maxPx = max(P.times_vector(x))
            minCx = min(C.times_vector(x))
            if minCx > 0:
                if outer_iterations % 1000 == 0:
                    print("outer iterations:", outer_iterations, 
                          ", eff eps: %.3f" % (maxPx/minCx-1))
                if maxPx <= (1+eps)*minCx:
                    report()
                    return [z/minCx for z in x]

            if all(
                    Pj_dot_p_hat_fn(j) * sum(exp_C_hat)
                    >
                    sum(exp_P_hat) * Cj_dot_c_hat_fn(j)
                    for j in range(n)
            ):
                report()
                print("infeasible")
                return None

        lambda_0 *= 1+eps

if __name__ == '__main__':
    random.seed(3141592)
    n = 15
    x = [random.random() if random.random() < 0.7 else 0 for _ in range(n)]
    P = [[random.choice((0, random.random()))
          for _ in range(n)] for _ in range(4*n)]
    C = [[random.choice((0, random.random()))
          for _ in range(n)] for _ in range(4*n)]
    P = [row for row in P if sum(a*b for (a, b) in zip(x, row)) != 0]
    C = [row for row in C if sum(a*b for (a, b) in zip(x, row)) != 0]
    for i in range(len(P)):
        total = sum(a*b for (a, b) in zip(x, P[i]))
        P[i] = [0.9999*a/total for a in P[i]]
    for i in range(len(C)):
        total = sum(a*b for (a, b) in zip(x, C[i]))
        C[i] = [1.0001*a/total for a in C[i]]

    def matrix(Q):
        M = Matrix()
        for row in range(len(Q)):
            for col in range(len(Q[row])):
                if Q[row][col] != 0:
                    M.add_entry(row, col, Q[row][col])
        return M

    str_vector = lambda v: " ".join("%5.3f" % z for z in v)
    str_matrix = lambda m: "\n".join(str_vector(row) for row in m)
    # print(str_matrix(P), " P\n")
    # print(str_matrix(C), " C\n")

    P, C = matrix(P), matrix(C)
    y = solve(P, C, 0.03)
    if y is not None:
        Py = P.times_vector(y)
        Cy = C.times_vector(y)
        print("x:", str_vector(x))
        print("y:", str_vector(y))
        print("min Cy:", min(Cy))
        print("max Py:", max(Py))
        print("eff. eps.:", max(Py)/min(Cy) - 1.0)
