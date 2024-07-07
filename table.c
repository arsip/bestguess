/*  -*- Mode: C; -*-                                                         */
/*                                                                           */
/*  table.c   Dynamically expanding tables of records                        */
/*                                                                           */
/* COPYRIGHT (c) Jamie A. Jennings, 2024                                     */

/*

  WARNING: NOT THREAD SAFE
  
*/

#include <string.h>
#include "table.h"
#include "logging.h"

// Supply zero as the 'max_capacity' to have no limit.
//
// The 'malloc' attribute enables the C compiler to do some
// optimizations that depend on this fact: The pointer returned does
// not alias other active pointers.
//
__attribute__((malloc, alloc_size(1, 3)))
Table *Table_new(int initial_capacity,
		 int max_capacity,
		 size_t record_size) {

  if ((initial_capacity <= 0) ||
      (max_capacity < 0) ||
      (record_size == 0) ||
      ((max_capacity != 0) && (max_capacity < initial_capacity))) {
    warn("table", "Invalid argument to Table_new");
    return NULL;
  }
  Table *tbl = malloc(sizeof(Table));
  if (!tbl) {
    warn("table", "Out of memory");
    return NULL;
  }
  tbl->records = malloc(initial_capacity * record_size);
  if (!tbl->records) {
    free(tbl);
    warn("table", "Out of memory");
    return NULL;
  }
  tbl->next = 0;
  tbl->capacity = initial_capacity;
  tbl->max_capacity = max_capacity;
  tbl->record_size = record_size;
  return tbl;
}

void Table_free(Table *tbl) {
  if (tbl) {
    if (tbl->records) free(tbl->records);
    free(tbl);
  }
  return;
}

/*
  Sets the '*record' arg to point to a new record inside 'tbl'.

  NOT THREAD SAFE.

  And, the pointer returned is valid only until the next call to
  Table_add().  Proper single-threaded usage looks like this:

    my_struct *s = (my_struct) = Table_add_unsafe(tbl, NULL);
    if (!s) ... Handle the error ...
    s->field1 = datum1;
    s->field2 = datum2;
    // Do not store 's' anywhere!
*/
void *Table_add_unsafe(Table *tbl, int *index) {
  if (!tbl) return NULL;

  if (tbl->next == tbl->capacity) {

    if (tbl->next == tbl->max_capacity) return NULL;

    size_t newcap = 2 * tbl->capacity;
    if (tbl->max_capacity)
      if (newcap > (size_t) tbl->max_capacity)
	newcap = tbl->max_capacity;

    // Ensure we are adding at least one new record
    assert(((ssize_t) tbl->next) < (ssize_t) newcap);

    void *temp = realloc(tbl->records, newcap * tbl->record_size);
    if (!temp) {
      warn("table", "Out of memory");
      return NULL;
    }

    tbl->records = temp;
    tbl->capacity = newcap;

    inform("table", "Extended table to capacity %d records (%d in use)",
	   tbl->capacity, tbl->next);
  }
  
  int i = tbl->next++;
  if (index) *index = i;
  return tbl->records + (i * tbl->record_size);
}

// Table_add() copies the contents of 'record'.  Returns the index of
// the record added, which is always non-negative, or an error code
// (always negative).
//
// For a thread-safe implementation, we would need to lock the table,
// or at least the new record we are adding.  We do not do this today.
// Consequently, if there are multiple threads, the pointer 'loc' can
// become invalid between the time we get it (from Table_add_unsafe)
// and the time we use it (in memcpy).
//
int Table_add(Table *tbl, void *record) {
  if (!tbl || !record) return Table_null;

  // Data structure consistency checks
  assert(tbl->records);
  assert((tbl->next >= 0) && (tbl->capacity > 0));
  assert(tbl->next <= tbl->capacity);

  int index;
  void *loc = Table_add_unsafe(tbl, &index);
  if (loc) {
    memcpy(loc, record, tbl->record_size);
    return index;
  }

  // Else warn about whatever error occurred
  if (tbl->max_capacity) {
    if (tbl->next == tbl->max_capacity) {
      warn("table", "Table full at capacity %d", tbl->max_capacity);
      return Table_full;
    }
    // The only other case for a NULL return is OOM, which has already
    // been warned about.  We just need to return the code for that.
    return Table_OOM;
  }

  warn("table", "Unexpected condition");
  return Table_error;
}

/*
  NOT THREAD SAFE.

  And, the pointer returned is valid only until the next call to
  Table_add().  Proper single-threaded usage looks like this:

    my_struct *s = (my_struct) Table_get_unsafe(tbl, idx);
    var1 = s->field1;
    var2 = s->field2;
    // Do not store 's' anywhere!
*/
void *Table_get_unsafe(Table *tbl, int i) {
  if (!tbl || (i < 0) || (i >= tbl->next)) return NULL;
  return tbl->records + (i * tbl->record_size);
}

int Table_get(Table *tbl, int i, void *record) {
  void *loc = Table_get_unsafe(tbl, i);
  if (loc) {
    memcpy(record, loc, tbl->record_size);
    return i;
  }
  if (!tbl || !record) {
    warn("table", "Required arg is NULL");
    return Table_null;
  }
  if ((i < 0) || (i >= tbl->next)) {
    warn("table", "Index out of range");
    return Table_notfound;
  }
  warn("table", "Unexpected condition");
  return Table_error;
}

int Table_set_size(Table *tbl, int value) {
  if (!tbl) return Table_null;
  if ((value < 0) || (value > tbl->capacity)) return Table_notfound;
  tbl->next = value;
  return value;
}

/*
  On the first call, supply -1 for '*prev'.  Same restrictions on use
  of returned pointer as for Table_get_unsafe().
 */
void *Table_iter (Table *tbl, int *prev) {
  if (!prev) return NULL;
  return Table_get_unsafe(tbl, ++(*prev));
}

int Table_size(Table *tbl) {
  if (!tbl) return Table_null;
  return tbl->next;
}

int Table_capacity(Table *tbl) {
  if (!tbl) return Table_null;
  return tbl->capacity;
}

/* ----------------------------------------------------------------------------- */
/* Stack interface                                                               */
/* ----------------------------------------------------------------------------- */

int Table_empty(Table *tbl) {
  if (!tbl) return Table_null;
  return (Table_size(tbl) == 0);
}
  
// Synonym, for when we use the table like a stack
int Table_push(Table *tbl, void *record) {
  return Table_add(tbl, record);
}

// NOTE: Returned pointer is only valid until the next Table_add().
void *Table_pop(Table *tbl) {
  if (!tbl || (tbl->next < 1)) return NULL;
  tbl->next--;
  return tbl->records + (tbl->next * tbl->record_size);
}

// NOTE: Returned pointer is only valid until the next Table_add().
void *Table_top(Table *tbl) {
  if (!tbl || (tbl->next < 1)) return NULL;
  return tbl->records + ((tbl->next - 1) * tbl->record_size);
}

