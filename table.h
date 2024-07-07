/*  -*- Mode: C; -*-                                                         */
/*                                                                           */
/*  table.h   Dynamically expanding tables of records                        */
/*                                                                           */
/* COPYRIGHT (c) Jamie A. Jennings, 2024                                     */

#ifndef table_h
#define table_h

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef enum Table_Error {
  Table_null,
  Table_full,
  Table_OOM,
  Table_notfound,
  Table_error
} Symbol_Namespace;

typedef struct Table {
  char   *records;
  size_t  record_size;
  int     next;
  int     capacity;
  int     max_capacity;
} Table;

Table *Table_new(int initial_capacity,
		 int max_capacity,
		 size_t record_size);

void   Table_free(Table *tbl);

int    Table_add(Table *tbl, void *record);
void  *Table_add_unsafe(Table *tbl, int *index);

int    Table_get(Table *tbl, int i, void *record);
void  *Table_get_unsafe(Table *tbl, int i);

void  *Table_iter(Table *tbl, int *prev);
int    Table_size(Table *tbl);
int    Table_capacity(Table *tbl);
int    Table_set_size(Table *tbl, int value);

int    Table_empty(Table *tbl);
int    Table_push(Table *tbl, void *record);
void  *Table_pop(Table *tbl);
void  *Table_top(Table *tbl);

#endif
