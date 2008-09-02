#include <stdlib.h>
#include <stdio.h>

double read_first_triple(int &i, int &j, int &k) {
  int result = scanf("%d %d %d\n", &i, &j, &k);
  if (result == 3) return k;
  if (result == EOF) return 0;
  fprintf(stderr, "error reading input\n");
  exit(-1);
}

double read_triple(int &i, int &j, double &d) {
  int result = scanf("%d %d %lg\n", &i, &j, &d);
  if (result == 3) return d;
  if (result == EOF) return 0;
  fprintf(stderr, "error reading input\n");
  exit(-1);
}

struct Entry {
  int i, j;
  double val;

  double read() {
    return read_triple(i, j, val);
  }
};

struct Row {
  int n, size;
  Entry *entries;
 
  Row() {
    n = 0;
    size = 8;
    entries = new Entry[size];
    if (! entries) {
      fprintf(stderr, "out of memory reading input\n");
      exit(-1);
    }
  }

  void grow() {
    int new_size = size*2;
    Entry *new_entries = new Entry[new_size];
    if (! new_entries) {
      fprintf(stderr, "out of memory reading input\n");
      exit(-1);
    }
    for (int i = 0;  i < n;  ++i) new_entries[i] = entries[i];
    delete[] entries;
    entries = new_entries;
    size = new_size;
  }

  void add(Entry &e) {
    if (n == size) grow();
    entries[n] = e;
    ++n;
  }
};


main() {
  int rows, cols, ones;

  if (! read_first_triple(rows, cols, ones)) {
    fprintf(stderr, "error reading first line\n");
    exit(-1);
  }

  fprintf(stdout, "rows %d cols %d nonzeros %d\n", rows, cols, ones);

  Row *matrix = new Row[rows];
  if (! matrix) {
    fprintf(stderr, "out of memory for matrix\n");
    exit(-1);
  }

  Entry e;

  while (e.read()) {
    if (e.i >= rows) {
      fprintf(stderr, "row %d out of range\n", e.i);
      exit(-1);
    }
    if (e.j >= cols) {
      fprintf(stderr, "col %d out of range\n", e.j);
      exit(-1);
    }
    matrix[e.i].add(e);
  }

  printf("Maximize\n");
  printf(" obj: x0");
  for (int i = 1;  i < cols;  i++)
    printf("+x%d", i);
  printf("\nSubject To\n");
  for (int i = 0;  i < rows;  i++) {
    if (matrix[i].n > 0) {
      printf(" c%d: %lg x%d", i,matrix[i].entries[0].val,matrix[i].entries[0].j);
      for (int k = 1;  k < matrix[i].n;  k++)
	printf("+ %lg x%d", matrix[i].entries[k].val, matrix[i].entries[k].j);
      printf(" <= 1\n");
    }
  }
  printf("Bounds\n");
  for (int i = 0;  i < cols;  i++)
    printf(" 0 <= x%d\n", i);
  printf("End\n");
}
