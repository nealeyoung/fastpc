#!/usr/bin/env python3.4

from math import log, ceil


class Entry:
    def __init__(self, row, col, value):
        self.row = row
        self.col = col
        self.value = value
        self.top = 2**ceil(log(value, 2))


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
        self.restore()

    def restore(self):
        for e in self.entries:
            e.removed = False

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
