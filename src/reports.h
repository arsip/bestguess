//  -*- Mode: C; -*-                                                       
// 
//  reports.h
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#ifndef reports_h
#define reports_h

#include "bestguess.h"
#include "stats.h"

// How many input files of data are we willing to read?
#define MAXDATAFILES 400

#define XReports(X)                                                             \
  /* REPORT_NONE must be first */                                               \
   X(REPORT_NONE,    "none",    "No report")                                    \
   X(REPORT_BRIEF,   "brief",   "Brief report with wall clock and CPU time")    \
   X(REPORT_SUMMARY, "summary", "Summary as when data was collected (default)") \
   X(REPORT_FULL,    "full",    "Summary and distribution analysis)")           \
  /* REPORT_ERROR must be last */                                               \
   X(REPORT_ERROR,    NULL, "SENTINEL"    )    

#define FIRST(a, b, c) a,
typedef enum { XReports(FIRST) } ReportCode;
#undef FIRST
extern const char *ReportOptionName[];
extern const char *ReportOptionDesc[];

char       *report_options(void);
void        free_report_options(void);
ReportCode  interpret_report_option(const char *op);

void   report(Usage *usage);
Usage *read_input_files(int argc, char **argv);

void announce_command(const char *cmd, int index, const char *fmt, int maxlen);
void print_summary(Summary *s, bool briefly);
void print_graph(Summary *s, Usage *usagedata, int start, int end);
void print_overall_summary(Summary *summaries[], int start, int end);
void print_descriptive_stats(Summary *s);
void print_boxplots(Summary *summaries[], int start, int end);

#endif
