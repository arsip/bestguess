//  -*- Mode: C; -*-                                                       
// 
//  stats.c  Summary statistics
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "bestguess.h"
#include "utils.h"
#include "stats.h"
#include <string.h>
#include <math.h>
#include <assert.h>

/* 
   Note: For a log-normal distribution, the geometric mean is the
   median value and is a more useful measure of the "typical" value
   than is the arithmetic mean.  We may not have a log normal
   distribution, however.

   If we had to choose a single number to characterize a distribution
   of runtimes, the mode (or modes) may be the most useful.  It
   represents the most common value, after all!

   However, if we are most concerned with the long tail, then the 99th
   percentile value should be highlighted.

   Given a choice between either median or mean, the median of a
   right-skewed distribution is typically the closer of the two to the
   mode.

 */

#define MEDIAN_SELECT(usage, fc, indices)				\
  ((config.runs == (config.runs/2) * 2) 				\
   ?									\
   ((get_int64(usage, indices[config.runs/2 - 1], fc) +			\
     get_int64(usage, indices[config.runs/2], fc)) / 2)			\
   :									\
   (get_int64(usage, indices[config.runs/2], fc)))

static int64_t avg(int64_t a, int64_t b) {
  return (a + b) / 2;
}

#define VALUEAT(i, fc) (get_int64(usage, indices[i], fc))
#define WIDTH(i, j, fc) (VALUEAT(j, fc) - VALUEAT(i, fc))

// "Half sample" technique for mode estimation.  The base cases are
// when number of samples, 𝑛, is 1, 2, or 3.  When 𝑛 > 3, there is a
// recursive case that computes the mode of ℎ = 𝑛/2 samples.  To
// choose which 𝑛/2 samples, we examine every interval [i, i+ℎ) to
// find the one with the smallest width.  That sequence of samples is
// the argument to the recursive call.
//
// See https://arxiv.org/abs/math/0505419
//
// Note that the data must be sorted, and for that we use 'indices'.
//
static int64_t estimate_mode(Usage *usage,
			     FieldCode fc,
			     int *indices) {
  // n = number of samples being examined
  // h = size of "half sample" (floor of n/2)
  // idx = start index of samples being examined
  // limit = end index one beyond last sample being examined
  int idx, limit, n, h;
  int64_t wmin = 0;
  idx = 0;
  n = config.runs;
  
 tailcall:
  if (n == 1) {
    return VALUEAT(idx, fc);
  } else if (n == 2) {
    return avg(VALUEAT(idx, fc), VALUEAT(idx+1, fc));
  } else if (n == 3) {
    // Break a tie in favor of lower value
    if (WIDTH(idx, idx+1, fc) <= WIDTH(idx+1, idx+2, fc))
      return avg(VALUEAT(idx, fc), VALUEAT(idx+1, fc));
    else
      return avg(VALUEAT(idx+1, fc), VALUEAT(idx+2, fc));
  }
  h = n / 2;	     // floor
  wmin = WIDTH(idx, idx+h, fc);
  // Search for half sample with smallest width
  limit = idx+h;
  for (int i = idx+1; i < limit; i++) {
    // Break ties in favor of lower value
    if (WIDTH(i, i+h, fc) < wmin) {
      wmin = WIDTH(i, i+h, fc);
      idx = i;
    }
  }
  n = h+1;
  goto tailcall;
}

// Returns -1 when there are insufficient samples
static int64_t percentile(int pct,
			  Usage *usage,
			  FieldCode fc,
			  int *indices) {
  if (pct < 90) PANIC("Error: refuse to calculate percentiles less than 90");
  if (pct > 99) PANIC("Error: unable to calculate percentiles greater than 99");
  // Number of samples needed for a percentile calculation
  int samples = 100 / (100 - pct);
  if (config.runs < samples) return -1;
  int idx = config.runs - (config.runs / samples);
  return get_int64(usage, indices[idx], fc);
}

// Produce a statistical summary (stored in 'meas') over all runs.
// Time values are single int64_t fields storing microseconds.
//
static void measure(Usage *usage,
		    int first,
		    int last,
		    FieldCode fc,
		    comparator compare,
		    measures *m) {
  int runs = last - first + 1;
  if (runs < 1) PANIC("no data to analyze");
  int *indices = malloc(runs * sizeof(int));
  if (!indices) PANIC_OOM();
  for (int i = 0; i < runs; i++) indices[i] = first + i;
  sort(indices, runs, sizeof(int), usage, compare);
  m->median = MEDIAN_SELECT(usage, fc, indices);
  m->min = get_int64(usage, indices[0], fc);
  m->max = get_int64(usage, indices[runs - 1], fc);
  m->mode = estimate_mode(usage, fc, indices);
  m->pct95 = percentile(95, usage, fc, indices);
  m->pct99 = percentile(99, usage, fc, indices);
  free(indices);
  return;
}

static summary *new_summary(void) {
  summary *s = calloc(1, sizeof(summary));
  if (!s) PANIC_OOM();
  return s;
}

void free_summary(summary *s) {
  free(s->cmd);
  free(s->shell);
  free(s);
}

// (1) 'cmd' is a filter that usage records must match exactly
// (2) All records with same 'cmd' must be contiguous
// (3) We assume that 'shell' is the same for each execution of 'cmd'
summary *summarize(char *cmd, Usage *usage) {
  summary *s = new_summary();
  s->cmd = strndup(cmd, MAXCMDLEN);

  int first = -1;
  for (int i = 0; i < usage->next; i++) {
    if (strncmp(cmd, get_string(usage, i, F_CMD), MAXCMDLEN) == 0) {
      first = i;
      break;
    }
  }
  // Maybe there was no matching data?
  if (first == -1) return s;

  s->cmd = strndup(get_string(usage, first, F_CMD), MAXCMDLEN);
  s->shell = strndup(get_string(usage, first, F_SHELL), MAXCMDLEN);

  int last = first;
  for (int i = first; i < usage->next; i++) {
    if (strncmp(cmd, get_string(usage, i, F_CMD), MAXCMDLEN) != 0) break;
    s->runs++;
    s->fail_count += (get_int64(usage, i, F_CODE) != 0);
    last = i;
  }

  measure(usage, first, last, F_TOTAL, compare_totaltime, &s->total);
  measure(usage, first, last, F_USER, compare_usertime, &s->user);
  measure(usage, first, last, F_SYSTEM, compare_systemtime, &s->system);
  measure(usage, first, last, F_MAXRSS, compare_maxrss, &s->maxrss);
  measure(usage, first, last, F_VCSW, compare_vcsw, &s->vcsw);
  measure(usage, first, last, F_ICSW, compare_icsw, &s->icsw);
  measure(usage, first, last, F_TCSW, compare_tcsw, &s->tcsw);
  measure(usage, first, last, F_WALL, compare_wall, &s->wall);

  return s;
}

// -----------------------------------------------------------------------------
// Z score calculation via table
// -----------------------------------------------------------------------------

// Cumulative (less than Z) Z-score calculation.  The table here gives
// the probability that a statistic is between negative infinity and Z.
//
// See e.g. https://en.wikipedia.org/wiki/Standard_normal_table
//
// Values -4.09..4.09 map to table indexes 0..817.
//
// Divide Ztable[index] by 10^4 to get the actual value.
//
// E.g. the Z-score for -4.05 is at index -405+409=4. Ztable[4] = 3.
// This value is the first 3 in the first row below.  The Z-score for
// -4.05 is 3/10^4 = 0.00003.
//
static const int
Ztable[] = {2, 2, 2, 2, 3, 3, 3, 3, 3, 3,
	    3, 3, 4, 4, 4, 4, 4, 4, 5, 5,
	    5, 5, 5, 6, 6, 6, 6, 7, 7, 7,
	    8, 8, 8, 8, 9, 9, 10, 10, 10, 11,
	    11, 12, 12, 13, 13, 14, 14, 15, 15, 16,
	    17, 17, 18, 19, 19, 20, 21, 22, 22, 23,
	    24, 25, 26, 27, 28, 29, 30, 31, 32, 34,
	    35, 36, 38, 39, 40, 42, 43, 45, 47, 48,
	    50, 52, 54, 56, 58, 60, 62, 64, 66, 69,
	    71, 74, 76, 79, 82, 84, 87, 90, 94, 97,
	    100, 104, 107, 111, 114, 118, 122, 126, 131, 135,
	    139, 144, 149, 154, 159, 164, 169, 175, 181, 187,
	    193, 199, 205, 212, 219, 226, 233, 240, 248, 256,
	    264, 272, 280, 289, 298, 307, 317, 326, 336, 347,
	    357, 368, 379, 391, 402, 415, 427, 440, 453, 466,
	    480, 494, 508, 523, 539, 554, 570, 587, 604, 621,
	    639, 657, 676, 695, 714, 734, 755, 776, 798, 820,
	    842, 866, 889, 914, 939, 964, 990, 1017, 1044, 1072,
	    1101, 1130, 1160, 1191, 1222, 1255, 1287, 1321, 1355, 1390,
	    1426, 1463, 1500, 1539, 1578, 1618, 1659, 1700, 1743, 1786,
	    1831, 1876, 1923, 1970, 2018, 2068, 2118, 2169, 2222, 2275,
	    2330, 2385, 2442, 2500, 2559, 2619, 2680, 2743, 2807, 2872,
	    2938, 3005, 3074, 3144, 3216, 3288, 3362, 3438, 3515, 3593,
	    3673, 3754, 3836, 3920, 4006, 4093, 4182, 4272, 4363, 4457,
	    4551, 4648, 4746, 4846, 4947, 5050, 5155, 5262, 5370, 5480,
	    5592, 5705, 5821, 5938, 6057, 6178, 6301, 6426, 6552, 6681,
	    6811, 6944, 7078, 7215, 7353, 7493, 7636, 7780, 7927, 8076,
	    8226, 8379, 8534, 8692, 8851, 9012, 9176, 9342, 9510, 9680,
	    9853, 10027, 10204, 10383, 10565, 10749, 10935, 11123, 11314, 11507,
	    11702, 11900, 12100, 12302, 12507, 12714, 12924, 13136, 13350, 13567,
	    13786, 14007, 14231, 14457, 14686, 14917, 15151, 15386, 15625, 15866,
	    16109, 16354, 16602, 16853, 17106, 17361, 17619, 17879, 18141, 18406,
	    18673, 18943, 19215, 19489, 19766, 20045, 20327, 20611, 20897, 21186,
	    21476, 21770, 22065, 22363, 22663, 22965, 23270, 23576, 23885, 24196,
	    24510, 24825, 25143, 25463, 25785, 26109, 26435, 26763, 27093, 27425,
	    27760, 28096, 28434, 28774, 29116, 29460, 29806, 30153, 30503, 30854,
	    31207, 31561, 31918, 32276, 32636, 32997, 33360, 33724, 34090, 34458,
	    34827, 35197, 35569, 35942, 36317, 36693, 37070, 37448, 37828, 38209,
	    38591, 38974, 39358, 39743, 40129, 40517, 40905, 41294, 41683, 42074,
	    42465, 42858, 43251, 43644, 44038, 44433, 44828, 45224, 45620, 46017,
	    46414, 46812, 47210, 47608, 48006, 48405, 48803, 49202, 49601,
	    50000,
	    50399, 50798, 51197, 51595, 51994, 52392, 52790, 53188, 53586, 
	    53983, 54380, 54776, 55172, 55567, 55962, 56360, 56749, 57142, 57535, 
	    57926, 58317, 58706, 59095, 59483, 59871, 60257, 60642, 61026, 61409, 
	    61791, 62172, 62552, 62930, 63307, 63683, 64058, 64431, 64803, 65173, 
	    65542, 65910, 66276, 66640, 67003, 67364, 67724, 68082, 68439, 68793, 
	    69146, 69497, 69847, 70194, 70540, 70884, 71226, 71566, 71904, 72240, 
	    72575, 72907, 73237, 73565, 73891, 74215, 74537, 74857, 75175, 75490, 
	    75804, 76115, 76424, 76730, 77035, 77337, 77637, 77935, 78230, 78524, 
	    78814, 79103, 79389, 79673, 79955, 80234, 80511, 80785, 81057, 81327, 
	    81594, 81859, 82121, 82381, 82639, 82894, 83147, 83398, 83646, 83891, 
	    84134, 84375, 84614, 84849, 85083, 85314, 85543, 85769, 85993, 86214, 
	    86433, 86650, 86864, 87076, 87286, 87493, 87698, 87900, 88100, 88298, 
	    88493, 88686, 88877, 89065, 89251, 89435, 89617, 89796, 89973, 90147, 
	    90320, 90490, 90658, 90824, 90988, 91149, 91308, 91466, 91621, 91774, 
	    91924, 92073, 92220, 92364, 92507, 92647, 92785, 92922, 93056, 93189, 
	    93319, 93448, 93574, 93699, 93822, 93943, 94062, 94179, 94295, 94408, 
	    94520, 94630, 94738, 94845, 94950, 95053, 95154, 95254, 95352, 95449, 
	    95543, 95637, 95728, 95818, 95907, 95994, 96080, 96164, 96246, 96327, 
	    96407, 96485, 96562, 96638, 96712, 96784, 96856, 96926, 96995, 97062, 
	    97128, 97193, 97257, 97320, 97381, 97441, 97500, 97558, 97615, 97670, 
	    97725, 97778, 97831, 97882, 97932, 97982, 98030, 98077, 98124, 98169, 
	    98214, 98257, 98300, 98341, 98382, 98422, 98461, 98500, 98537, 98574, 
	    98610, 98645, 98679, 98713, 98745, 98778, 98809, 98840, 98870, 98899, 
	    98928, 98956, 98983, 99010, 99036, 99061, 99086, 99111, 99134, 99158, 
	    99180, 99202, 99224, 99245, 99266, 99286, 99305, 99324, 99343, 99361, 
	    99379, 99396, 99413, 99430, 99446, 99461, 99477, 99492, 99506, 99520, 
	    99534, 99547, 99560, 99573, 99585, 99598, 99609, 99621, 99632, 99643, 
	    99653, 99664, 99674, 99683, 99693, 99702, 99711, 99720, 99728, 99736, 
	    99744, 99752, 99760, 99767, 99774, 99781, 99788, 99795, 99801, 99807, 
	    99813, 99819, 99825, 99831, 99836, 99841, 99846, 99851, 99856, 99861, 
	    99865, 99869, 99874, 99878, 99882, 99886, 99889, 99893, 99896, 99900, 
	    99903, 99906, 99910, 99913, 99916, 99918, 99921, 99924, 99926, 99929, 
	    99931, 99934, 99936, 99938, 99940, 99942, 99944, 99946, 99948, 99950, 
	    99952, 99953, 99955, 99957, 99958, 99960, 99961, 99962, 99964, 99965, 
	    99966, 99968, 99969, 99970, 99971, 99972, 99973, 99974, 99975, 99976, 
	    99977, 99978, 99978, 99979, 99980, 99981, 99981, 99982, 99983, 99983, 
	    99984, 99985, 99985, 99986, 99986, 99987, 99987, 99988, 99988, 99989, 
	    99989, 99990, 99990, 99990, 99991, 99991, 99992, 99992, 99992, 99992, 
	    99993, 99993, 99993, 99994, 99994, 99994, 99994, 99995, 99995, 99995, 
	    99995, 99995, 99996, 99996, 99996, 99996, 99996, 99996, 99997, 99997, 
	    99997, 99997, 99997, 99997, 99997, 99997, 99998, 99998, 99998, 99998};

// Since Z is a name for the integers, the name zzscore is apt.
int zzscore(int scaled_z) {
  if (scaled_z < -409) return 0;
  if (scaled_z > 409) return 100000;
  return Ztable[scaled_z + 409];
}

double zscore(double z) {
  int scaled_z = round(z * 1000.0);
  int units = scaled_z % 10;
  int lower = scaled_z - ((units < 0) ? (units + 10) : units);
  int upper = lower + 10;
//   if (units) printf("units = %d, low[%04d] = %d, high[%04d] = %d\n", units,
// 		    lower, zzscore(lower/10),
// 		    upper, zzscore(upper/10));
  double diff = zzscore(upper/10) - zzscore(lower/10);
  if (units < 0) units = 10 + units;
  double score = zzscore(lower/10) + ((double) units/10.0 * diff);
  assert(zzscore(upper/10) >= score);
  assert(zzscore(lower/10) <= score);
  return score / 10e4;
}

__attribute__((unused))
void print_zscore_table(void) {
  int rows;
  printf("             %7.2f", 0.0);
  for (int b = 1; b <= 9; b++)
    printf("  %7.2f", b * 0.01);
  printf("\n");
  for (int a = -41; a <= 41; a++) {
    rows = (a == 0) ? 2 : 1;
    // The zero row prints twice, once for the values -0.00..-0.09 and
    // once for 0.00..0.09.
    for (int j = 0; j < rows; j++) {
      printf("z = %4.1f  :  ", (double) a / 10.0);
      for (int b = 0; b <= 9; b++) {
	int zz = a * 10 + ((a <= 0) ? -b : b);
	if (j == 1) zz = a * 10 + b;
	printf("%7.5f  ", zscore(zz / 100.0));
      }
      printf("\n");
    }
  }
  printf("\n");
}

// -----------------------------------------------------------------------------
// Testing a sample distribution for normality
// -----------------------------------------------------------------------------

// https://en.wikipedia.org/wiki/Anderson–Darling_test
//
// Input: Xi are samples, order from smallest to largest
//        n is the number of samples
//
// Estimate mean of sample distribution: μ = (1/n) Σ(Xi)
// Estimate variance: σ² = (1/(n-1)) Σ(Xi-μ)²
//
// Standardize Xi as: Yi = (Xi-μ)/σ
// Compute Z-score for each Yi using a table: call them Zi
//
// To compare our distribution (the samples Xi) to a normal
// distribution, we compute the distance A² as follows:
//
// A² = -n - (1/n) Σ( (2i-1)ln(Zi) + (2(n-i)+1)ln(1-Zi) )
//
// When neither the mean or variance of the sample distribution are
// known, a correction is applied to give a final distance measurement
// of: A⃰² = A²(1 + 4/n + 25/n²)
//
// I'll use d (for distance) to represent the quantity A⃰².
//
// The hypothesis that our sample distribution is normal is rejected
// is computed by looking up d in the table below, using the row
// appropriate for the number of samples.  The column headings are
// p-values.  E.g. for 20 samples, if d > 0.969, then we can reject
// the hypothesis that we have a normal distribution with p=0.01,
// i.e. with 99% confidence.
//
// 	n	15%	10%	5%	2.5%	1%
//     10	0.514	0.578	0.683	0.779	0.926
//     20	0.528	0.591	0.704	0.815	0.969
//     50	0.546	0.616	0.735	0.861	1.021
//     100	0.559	0.631	0.754	0.884	1.047
//     ∞	0.576	0.656	0.787	0.918	1.092
//
const int ADconfidence[30] = {   10,   514,   578,   683,   779,   926,
				 20,   528,   591,   704,   815,   969,
				 50,   546,   616,   735,   861,  1021,
				100,   559,   631,   754,   884,  1047,
				 -1,   576,   656,   787,   918,  1092  };

