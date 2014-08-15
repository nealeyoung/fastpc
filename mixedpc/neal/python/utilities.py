#!/usr/bin/env python3.4

import random
from sys import _getframe

from matrix import Matrix


class cell:
    def __init__(self, fn, *deps, name=""):
        self.destroyed = False
        self.name = name
        self.deps = deps
        self.fn = fn
        self.value = fn()
        self.hooks = []
        for d in self.deps:
            d.add_hook(self.update)

    def add_hook(self, hook):
        self.hooks.append(hook)

    def del_hook(self, hook):
        self.hooks.remove(hook)

    def reset(self):
        self.value = self.fn()

    def update(self, *args):
        old_value, self.value = self.value, self.fn()
        for t in self.hooks:
            t(self.value, old_value, self)

    def destroy(self):
        assert not self.destroyed
        self.destroyed = True
        for d in self.deps:
            d.del_hook(self.update)
        del self.fn
        del self.hooks
        self.update = lambda: None
        self.reset = lambda: None
                # del self.deps
        # del self.value

    def __repr__(self):
        name = self.name or '?'
        return "<cell %s %.3g>" % (name, self.value)


class cell_sum(cell):
    def __init__(self, deps, name=""):
        deps = tuple(deps)
        fn = lambda deps=deps: sum(d.value for d in deps)
        cell.__init__(self, fn, *deps, name=name)

    def reset(self):
        for d in self.deps:
            d.reset()
        self.value = self.fn()

    def update(self, new_cell_value, old_cell_value, *args):
        if self.destroyed:
            return
        old_sum = self.value
        self.value += new_cell_value - old_cell_value
        for t in self.hooks:
            t(self.value, old_sum, self)

    def recalibrate(self):
        if not self.destroyed:
            old_value, self.value = self.value, sum(d.value for d in self.deps)
            assert -0.01 < old_value/self.value - 1 < 0.01 \
                if self.value != 0 else -0.01 < old_value < 0.01, dump(
                    "self.name;old_value;self.value")

    def destroy(self):
        assert not self.destroyed
        # if self.name:
        #     print("destroy", self.name)
        for d in self.deps:
            d.destroy()
        self.deps = []
        cell.destroy(self)

    def __getitem__(self, i):
        return self.deps[i].value

    def __iter__(self):
        for d in self.deps:
            yield d.value


def dump(*exprs):
    callers_locals = dict(_getframe(1).f_locals)
    exprs = [e.strip() for e1 in exprs for e in e1.split(";")]
    values = [eval(e, globals(), callers_locals) for e in exprs]
    return "\n".join("%s = %s" % z for z in zip(exprs, values))


def random_P_C_x(n):
    x = [random.random() if random.random() < 0.2 else 0 for _ in range(n)]
    P = [[random.choice((0, random.random()))
          for _ in range(n)] for _ in range(n)]
    C = [[random.choice((0, random.random()))
          for _ in range(n)] for _ in range(n)]
    P = [row for row in P if sum(a*b for (a, b) in zip(x, row)) != 0]
    C = [row for row in C if sum(a*b for (a, b) in zip(x, row)) != 0]
    for i in range(len(P)):
        total = sum(a*b for (a, b) in zip(x, P[i]))
        P[i] = [_*0.99/total for _ in P[i]]
    for i in range(len(C)):
        total = sum(a*b for (a, b) in zip(x, C[i]))
        C[i] = [_*1.01/total for _ in C[i]]

    def matrix(Q):
        M = Matrix()
        for row in range(len(Q)):
            for col in range(len(Q[row])):
                if Q[row][col] != 0:
                    M.add_entry(row, col, Q[row][col])
        M.done_adding_entries()
        return M

    return matrix(P), matrix(C), x

str_vector = lambda v: " ".join("%5.3f" % z for z in v)

if __name__ == "__main__":
    x = list(range(10))
    cells = [cell(lambda i=i: x[i]) for i in range(len(x))]
    c_sum = cell_sum(cells)

    assert(c_sum.value == sum(x))

    x[0] = 1
    cells[0].update()
    assert(c_sum.value == sum(x))

    print("tests passed")
