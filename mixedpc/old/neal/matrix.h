#ifndef _MATRIX_H_
#define _MATRIX_H_

#include <list>

class Matrix {
 public:
  struct Entry;
  class Vector;

  Matrix();
  ~Matrix();

  // call add_entry repeatedly, then done_adding_entries, to initialize matrix
  void     add_entry            (int row, int col, double value);
  void     done_adding_entries  ();

  Vector*  row                  (int i);     
  Vector*  col                  (int i);     

  void     remove_entry         (Entry** entry);
  void     remove_col           (int col);
  void     remove_row           (int row);
  void     compact              ();

  int      n_rows               ();
  int      n_cols               ();
  int      n_non_empty_rows     ();
  int      n_non_empty_cols     ();
  void     dump                 ();
  void     restore              (); // undo all row/col removals

  struct Entry {
    Entry(int row, int col, double value) :
      _row (row), _col(col), _value(value), _removed(false) {};
    int     _row;
    int     _col;
    double  _value;
    bool    _removed;
  };

  class Vector {
  public:
    int     size        ();
    Entry** max_entry   ();
    Entry** first_entry (double threshold = 0);
    Entry** next_entry  (Entry** entry, double threshold = 0);
    bool    empty       () { return size() == 0; }

    ///////////////////   stuff below is private /////////////////

  protected:
    friend class Matrix;

    Entry** _storage;
    int     _storage_size;
    int     _n_high;
    int     _n_not_removed;

    Vector(int n) :
      _storage(new Entry*[n]),
      _storage_size(n),
      _n_high(0),
      _n_not_removed(0)
    {}
    ~Vector();

    void clear           ();
    void add             (Entry* entry);

    void compact         ();

    struct Comp;
    void sort            ();
  };

  std::list<Entry*> _entries;
  bool              _done_adding_entries;

  Vector**          _rows;
  Vector**          _cols;

  int               _n_rows;
  int               _n_cols;
  int               _n_non_empty_rows;
  int               _n_non_empty_cols;
};

#endif // _MATRIX_H_
