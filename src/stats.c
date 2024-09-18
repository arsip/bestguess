//  -*- Mode: C; -*-                                                       
// 
//  stats.c  Summary statistics
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "bestguess.h"
#include "utils.h"
#include "stats.h"
#include <math.h>
#include <assert.h>

/* 
   Note: For skewed distributions, the median is a more useful measure
   of the "typical" value than is the arithmetic mean. 

   But if we had to choose a single number to characterize a
   distribution of runtimes, the mode (or modes) may be the most
   useful as it represents the most common value.  That is, the mode
   is the most "typical" runtime.

   However, if we are most concerned with the long tail, then the 99th
   percentile value should be highlighted.

   Given a choice between either median or mean, the median of a
   right-skewed distribution is typically the closer of the two to the
   mode.

 */

// -----------------------------------------------------------------------------
// Testing a sample distribution for normality
// -----------------------------------------------------------------------------

// https://en.wikipedia.org/wiki/Anderson–Darling_test
//
// Input: Xi are samples, order from smallest to largest
//        n is the number of samples
//        i ranges from 1..n inclusive
//
// Estimate mean of sample distribution: μ = (1/n) Σ(Xi)
// Estimate variance: σ² = (1/(n-1)) Σ(Xi-μ)²
//
// Standardize Xi as: Yi = (Xi-μ)/σ
//
// Compute F(Yi) where F is the CDF for (in this case) the normal
// distribution, and is the Z-score for each Yi. Let Zi = F(Yi).
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
// Or this alternative: A⃰² = A²(1 + 0.75/n + 2.25/n²)
//
// We'll the alternative correction, and the term 'AD score' to mean
// the quantity A⃰².
//
// The hypothesis that our sample distribution is normal is rejected
// if the AD score.  Wikipedia cites the book below for the
// alternative correction mentioned above, and these critical values:
// 
//         p value  0.10   0.05   0.025   0.01  0.005
//  critical value  0.631  0.754  0.884  1.047  1.159
//
// Ralph B. D'Agostino (1986). "Tests for the Normal Distribution". In
// D'Agostino, R.B.; Stephens, M.A. (eds.). Goodness-of-Fit
// Techniques. New York: Marcel Dekker. ISBN 0-8247-7487-6.
//
// The calculate_p() function produces the needed critical values:
//                   p value  0.10    0.05    0.025   0.01    0.005
// calculated critical value  0.1001  0.0498  0.0238  0.0094  0.0050

// FWIW:
//
// https://support.minitab.com/en-us/minitab/help-and-how-to/statistics
// /basic-statistics/supporting-topics/normality/test-for-normality/
//
// "Anderson-Darling tends to be more effective in detecting
// departures in the tails of the distribution. Usually, if departure
// from normality at the tails is the major problem, many
// statisticians would use Anderson-Darling as the first choice."

//
// Calculating p-value for normal distribution based on AD score.  See
// e.g. https://real-statistics.com/non-parametric-tests/goodness-of-fit-tests/anderson-darling-test/
// or https://www.statisticshowto.com/anderson-darling-test/
//
// The null hypothesis is that the sample distribution is normal.  A
// low p-value is a high likelihood that the distribution is NOT
// normal.  E.g. p = 0.01 indicates a 1% chance that the distribution
// we examined is actually normal, or a 99% chance it is NOT normal.
//
static double calculate_p(double AD) {
  if (AD <= 0.2) return 1.0 - exp(-13.436 + 101.14*AD - 223.73*AD*AD);
  if (AD <= 0.34) return 1.0 - exp(-8.318 + 42.796*AD - 59.938*AD*AD);
  if (AD <  0.6) return exp(0.9177 - 4.279*AD - 1.38*AD*AD);
  else return exp(1.2937 - 5.709*AD + 0.0186*AD*AD);
}

//
// Wikipedia cites the book below for the claim that a minimum of 8
// data points is needed.
// https://en.wikipedia.org/wiki/Anderson–Darling_test#cite_note-RBD86-6
// 
// Ralph B. D'Agostino (1986). "Tests for the Normal Distribution". In
// D'Agostino, R.B.; Stephens, M.A. (eds.). Goodness-of-Fit
// Techniques. New York: Marcel Dekker. ISBN 0-8247-7487-6.
//
static double AD_from_Y(int n,		      // number of data points
			double *Y,	      // standardized ranked data
			double (F)(double)) { // CDF function
  assert(Y && (n > 0));
  double S = 0;
  for (int i = 1; i <= n; i++)
    S += (2*i-1) * log(F(Y[i-1])) + (2*(n-i)+1) * log(1.0-F(Y[i-1]));
  S = S/n;
  double A = -n - S;
  // Recommended correction factor for our critical p-values
  A = A * (1 + 0.75/n + 2.25/(n * n));
  return A;
}

static double AD_normality(int64_t *X,    // ranked data points
			   int n,         // number of data points
			   double mean,
			   double stddev) {
  assert(X && (n > 0));
  double *Y = malloc(n * sizeof(double));
  if (!Y) PANIC_OOM();
  for (int i = 0; i < n; i++) {
    // Y is a vector of studentized residuals
    Y[i] = ((double) X[i] - mean) / stddev;
    if (Y[i] != Y[i]) {
      PANIC("Got NaN.  Should not have attempted AD test because stddev is zero.");
    }
    // Extreme values (i.e. high Z scores) indicate a long tail, and
    // prevent the computation of a meaningful AD score.
    // Approximately 1 observation in 15,787 will be more than 4 std
    // deviations from the estimated mean per
    // https://en.wikipedia.org/wiki/68–95–99.7_rule
    if (fabs(Y[i]) > 4.0) {
      free(Y);
      return -1;
    }
  }
  double A = AD_from_Y(n, Y, normalCDF);
  free(Y);
  return A;
}

// -----------------------------------------------------------------------------
// Attributes of the distribution that we can calculate directly
// -----------------------------------------------------------------------------

static int64_t avg(int64_t a, int64_t b) {
  return (a + b) / 2;
}

// static int64_t median(int64_t *X, int n) {
//   assert(X && (n > 0));
//   int half = n / 2;
//   if (n == (n/2) * 2)
//     return avg(X[half - 1], X[half]);
//   return X[half];
// }

#define VALUEAT(i) (X[i])
#define WIDTH(i, j) (VALUEAT(j) - VALUEAT(i))

// "Half sample" technique for mode estimation.  The base cases are
// when number of samples, 𝑛, is 1, 2, or 3.  When 𝑛 > 3, there is a
// recursive case that computes the mode of ℎ = 𝑛/2 samples.  To
// choose which 𝑛/2 samples, we examine every interval [i, i+ℎ) to
// find the one with the smallest width.  That sequence of samples is
// the argument to the recursive call.
//
// See https://arxiv.org/abs/math/0505419
//
// Note that the data, X, must be sorted.
//
static int64_t estimate_mode(int64_t *X, int n) {
  assert(X && (n > 0));
  // n = size of sample being examined (number of observations)
  // h = size of "half sample" (floor of n/2)
  // idx = start index of samples being examined
  // limit = end index one beyond last sample being examined
  int64_t wmin = 0;
  int idx = 0;
  int limit, h;

 tailcall:
  if (n == 1) {
    return VALUEAT(idx);
  } else if (n == 2) {
    return avg(VALUEAT(idx), VALUEAT(idx+1));
  } else if (n == 3) {
    // Break a tie in favor of lower value
    if (WIDTH(idx, idx+1) <= WIDTH(idx+1, idx+2))
      return avg(VALUEAT(idx), VALUEAT(idx+1));
    else
      return avg(VALUEAT(idx+1), VALUEAT(idx+2));
  }
  h = n / 2;	     // floor
  wmin = WIDTH(idx, idx+h);
  // Search for half sample with smallest width
  limit = idx+h;
  for (int i = idx+1; i < limit; i++) {
    // Break ties in favor of lower value
    if (WIDTH(i, i+h) < wmin) {
      wmin = WIDTH(i, i+h);
      idx = i;
    }
  }
  n = h+1;
  goto tailcall;
}

// Returns -1 when there are insufficient observations for a 95th or
// 99th percentile request.  For quartiles, we make the best estimate
// we can with the data we have.
static int64_t percentile(int pct, int64_t *X, int n) {
  if (!X) PANIC_NULL();
  if (n < 1) PANIC("No data");
  switch (pct) {
    case 0:
      return X[0];		// Q0 = Min
    case 25:
      return X[n/4];		// Q1
      break;
    case 50:
      if (n & 0x1)		// Q2 = Median
	return X[n/2];
      return avg(X[(n/2) - 1], X[n/2]);
    case 75:
      return X[(3*n)/4];	// Q3
    case 95:
      if (n < 20) return -1;
      return X[n - n/20];
    case 99:
      if (n < 100) return -1;
      return X[n - n/100];
      break;
    case 100:
      return X[n - 1];
    default:
      PANIC("Error: percentile %d unimplemented", pct);
  }
}

// Estimate the sample mean: μ = (1/n) Σ(Xi)
static double estimate_mean(int64_t *X, int n) {
  assert(X && (n > 0));
  double sum = 0;
  for (int i = 0; i < n; i++)
    sum += X[i];
  return sum / n;
}

// Estimate the sample variance: σ² = (1/(n-1)) Σ(Xi-μ)²
// Return σ, the estimated standard deviation.
static double estimate_stddev(int64_t *X, int n, double est_mean) {
  assert(X && (n > 0));
  double sum = 0;
  for (int i = 0; i < n; i++)
    sum += pow(X[i] - est_mean, 2);
  return sqrt(sum / (n - 1));
}

static int *make_index(Usage *usage, int start, int end, Comparator compare) {
  assert(usage);
  int runs = end - start;
  if (runs <= 0) PANIC("invalid start/end");
  int *index = malloc(runs * sizeof(int));
  if (!index) PANIC_OOM();
  for (int i = 0; i < runs; i++) index[i] = start + i;
  sort(index, runs, sizeof(int), compare, usage);
  return index;
}

static int64_t *ranked_sample(Usage *usage,
			      int start,
			      int end,
			      FieldCode fc,
			      Comparator comparator) {
    int *index = make_index(usage, start, end, comparator);
    int runs = end - start;
    int64_t *X = malloc(runs * sizeof(double));
    if (!X) PANIC_OOM();
    for (int i = 0; i < runs; i++)
      X[i] = get_int64(usage, index[i], fc);
    free(index);
    return X;
}

// TODO: How close to zero is too small a stddev (or variance) for
// ADscore to be meaningful?

#define LOWMEAN_THRESHOLD 0.1	   // Good for integer number of microsecs
#define LOWSTDDEV_THRESHOLD 0.1    // Somewhat arbitrary
#define N_THRESHOLD 8		   // Per guidance on using AD test

static bool lowvariance(double mean, double stddev) {
  assert(stddev >= -1e-6);
  return (fabs(mean) < LOWMEAN_THRESHOLD)
    || (stddev < LOWSTDDEV_THRESHOLD);
}

//
// Produce a statistical summary (stored in 'm') over all runs.  Time
// values are single int64_t fields storing microseconds.
//
static void measure(Usage *usage,
		    int start,
		    int end,
		    FieldCode fc,
		    Comparator compare,
		    Measures *m) {
  int runs = end - start;
  if (runs < 1) PANIC("no data to analyze");
  int64_t *X = ranked_sample(usage, start, end, fc, compare);

  // Descriptive statistics of the data distribution
  m->mode = estimate_mode(X, runs);
  m->min = percentile(0, X, runs);
  m->Q1 = percentile(25, X, runs);
  m->median = percentile(50, X, runs);
  m->Q3 = percentile(75, X, runs);
  m->pct95 = percentile(95, X, runs);
  m->pct99 = percentile(99, X, runs);
  m->max = percentile(100, X, runs);

  // Estimates based on the data distribution 
  m->est_mean = estimate_mean(X, runs);
  if (runs > 1)
    m->est_stddev = estimate_stddev(X, runs, m->est_mean);
  else
    m->est_stddev = 0;

  // Compute Anderson-Darling distance from normality if we have
  // enough data points.  The literature suggests that 8 suffices.
  // Also, if the estimated stddev is too low, the ADscore is not
  // meaningful (and numerically unstable) so we skip it.
  //
  // Skew also depends on having a variance that is not "too low".
  m->code = 0;
  if (runs < N_THRESHOLD)
    SET(m->code, CODE_SMALLN);
  if (lowvariance(m->est_mean, m->est_stddev))
    SET(m->code, CODE_LOWVARIANCE);

  assert(m->skew == 0.0);
  if (m->code != 0) goto noscore;

  m->skew = (m->est_mean - m->median) / m->est_stddev;

  m->ADscore = AD_normality(X, runs, m->est_mean, m->est_stddev);
  // Off-the-charts z-scores are only detected when we calculate the
  // AD score.
  if (m->ADscore == -1) {
    SET(m->code, CODE_HIGHZ);
    goto noscore;
  }
  m->p_normal = calculate_p(m->ADscore);
  free(X);
  return;

 noscore:
  m->ADscore = -1;
  m->p_normal = -1;
  free(X);
  return;
}

static Summary *new_summary(void) {
  Summary *s = calloc(1, sizeof(Summary));
  if (!s) PANIC_OOM();
  return s;
}

void free_summary(Summary *s) {
  if (!s) return;
  free(s->cmd);
  free(s->shell);
  free(s);
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

// https://owlcalculator.com/statistics/normal-distribution-calculator/by-z-score
// 4.09, 0.99997842
// 4.10, 0.99997933
// 4.11, 0.99998021
// 4.12, 0.99998105
// 4.13, 0.99998185
// 4.14, 0.99998262
// 4.15, 0.99998337
//
// 4.50, 0.9999966
//


static int integer_normalCDF(int scaled_z) {
  if (scaled_z < -409) return 0;
  if (scaled_z > 409) return 100000;
  return Ztable[scaled_z + 409];
}

double normalCDF(double z) {
  int scaled_z = round(z * 1000.0);
  int units = scaled_z % 10;
  int lower = scaled_z - ((units < 0) ? (units + 10) : units);
  int upper = lower + 10;
  double diff = integer_normalCDF(upper/10) - integer_normalCDF(lower/10);
  if (units < 0) units = 10 + units;
  double value = integer_normalCDF(lower/10) + ((double) units/10.0 * diff);
  assert(integer_normalCDF(upper/10) >= value);
  assert(integer_normalCDF(lower/10) <= value);
  return value / 10e4;
}

// Current impl searches the table.  It's slow, but it isn't used
// often.  FUTURE: Maybe do something better here.
double inverseCDF(double alpha) {
  // E.g. when alpha is 0.9750, target index is 97500
  int target = round(alpha * 100000.0);
  if (target < 2) return -4.10;	   // precision limit of our table
  if (target > 99998) return 4.10; // precision limit of our table
  int d, distance = 100000;
  int answer = -12345;
  //printf("Alpha is %f, Target is %d\n", alpha, target);
  for (int x = -409; x <= 409; x++) {
    d = abs(integer_normalCDF(x) - target);
    if (d < distance) {
      answer = x;
      distance = d;
    }
  }
  if (answer == -12345)
    PANIC("Inverse CDF search failed for alpha = %f", alpha);
  return (double) answer / 100.0;
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
	printf("%7.5f  ", normalCDF(zz / 100.0));
      }
      printf("\n");
    }
  }
  printf("\n");
}

// -----------------------------------------------------------------------------
// Compute statistical summary of a sample (collection of observations)
// -----------------------------------------------------------------------------

//
// (1) Summarizing starts at index '*next'
// (2) And continues until 'cmd' changes
// (3) '*next' is set to the index where 'cmd' changed
// (4) We assume that 'shell' is the same for each execution of 'cmd'
//
Summary *summarize(Usage *usage, int *next) {
  if (!next) PANIC_NULL();
  // No data to summarize if 'usage' is NULL or 'next' is out of range
  if (!usage) return NULL;
  if ((*next < 0) || (*next >= usage->next)) return NULL;
  int i;
  char *cmd;
  int first = *next;
  Summary *s = new_summary();
  cmd = get_string(usage, first, F_CMD);
  s->cmd = strndup(cmd, MAXCMDLEN);
  s->shell = strndup(get_string(usage, first, F_SHELL), MAXCMDLEN);
  for (i = first; i < usage->next; i++) {
    if (strncmp(cmd, get_string(usage, i, F_CMD), MAXCMDLEN) != 0) break;
    s->runs++;
    s->fail_count += (get_int64(usage, i, F_CODE) != 0);
  }
  *next = i;
  measure(usage, first, i, F_TOTAL, compare_totaltime, &s->total);
  measure(usage, first, i, F_USER, compare_usertime, &s->user);
  measure(usage, first, i, F_SYSTEM, compare_systemtime, &s->system);
  measure(usage, first, i, F_MAXRSS, compare_maxrss, &s->maxrss);
  measure(usage, first, i, F_VCSW, compare_vcsw, &s->vcsw);
  measure(usage, first, i, F_ICSW, compare_icsw, &s->icsw);
  measure(usage, first, i, F_TCSW, compare_tcsw, &s->tcsw);
  measure(usage, first, i, F_WALL, compare_wall, &s->wall);

  return s;
}

// -----------------------------------------------------------------------------
// Inferential statistics
// -----------------------------------------------------------------------------

// On exit, ranks[i] is the rank of X[index[i]].  If the 'index' arg
// is NULL, ranks[i] is the rank of X[i].
static double *assign_ranks(int64_t *X, int *index, int N) {
  double *ranks = malloc(N * sizeof(double));
  if (!ranks) PANIC_OOM();
  ranks[0] = 1.0;
  int ties = 0;
  for (int i = 1; i < N; i++) {
    if ((index && (X[index[i]] == X[index[i - 1]]))
	|| (X[i] == X[i - 1])) {
      ties++;
      continue;
    }
    if (ties) {
      // Found the end of a set of tie values
      double avg = ((double) ties / 2.0) + i - ties;
      for (int j = 0; j < (ties + 1); j++) 
	ranks[i - j - 1] = avg;
      ties = 0;
    }
    ranks[i] = i + 1;
  }
  if (ties) {
    double avg = ((double) ties / 2.0) + N - ties;
    for (int j = 0; j < (ties + 1); j++) 
      ranks[N - j - 1] = avg;
  }

  if (DEBUG) {
    double totalrank = 0.0;
    for (int k = 0; k < N; k++)
      totalrank += ranks[k];
    printf("Total rank = %8.3f\n", totalrank);
    double expected = N * (N + 1) / 2.0;
    printf("Expected total rank = %8.3f\n", expected);
    if (expected != totalrank) PANIC("Rank total is incorrect");
    printf("Rank sum is correct at %f\n", totalrank);
  }
  return ranks;
}

// This comparator works when 'data' is an index vector and also when
// 'data' is NULL and there is no index.
static int i64_lt(const void *a, const void *b, void *data) {
  if (!data) {
    return *(const int64_t *)a - *(const int64_t *)b;
  }
  const int64_t *X = (const int64_t *) data;
  int64_t Xa = X[*(const int *)a];
  int64_t Xb = X[*(const int *)b];
  return Xa - Xb;
}

typedef struct XO {
  int64_t X;
  int origin;
} XO;

static int XO_lt(const void *a, const void *b, void *data) {
  if (data) PANIC("XO_lt does not work with an index vector");
  int64_t Xa = ((const XO *)a)->X;
  int64_t Xb = ((const XO *)b)->X;
  return Xa - Xb;
}

// TODO: If we decide to keep this calculation, make it efficient.
//  
// Requires start/end ranges for sample 1 and sample 2 are non-empty
// and do not overlap.  N.B. end index is not included in range.
RankedCombinedSample rank_combined_samples(Usage *usage,
					   int start1, int end1,
					   int start2, int end2,
					   FieldCode fc) {
//   printf("Ranking combined samples:\n");
//   printf("sample 1 [%d, %d)\nsample 2 [%d, %d)\n",
// 	 start1, end1, start2, end2);
//   printf("usage->next = %d\n", usage->next);
//   fflush(NULL);

  int n1 = end1 - start1;
  int n2 = end2 - start2;
  int N = n1 + n2;
  
  if ((n1 <= 0) || (n2 <= 0))
    PANIC("Invalid sample sizes");
  if (((start1 >= start2) && (start1 < end2))
      || ((end1 > start2) && (end1 < end2)))
    PANIC("Invalid sample index ranges in usage structure");

  // Must sort both X and the origins.
  XO *XOs = malloc(N * sizeof(XO));
  if (!XOs) PANIC_OOM();
  for (int i = 0; i < n1; i++) {
    XOs[i].X = get_int64(usage, start1 + i, fc);
    XOs[i].origin = 1;
  }
  for (int i = 0; i < n2; i++) {
    XOs[i + n1].X = get_int64(usage, start2 + i, fc);
    XOs[i + n1].origin = 2;
  }

  sort(XOs, N, sizeof(XO), XO_lt, NULL);

  int64_t *X = malloc(N * sizeof(int64_t));
  if (!X) PANIC_OOM();
  int *origin = malloc(N * sizeof(int));
  if (!origin) PANIC_OOM();
  for (int k = 0; k < N; k++) {
    X[k] = XOs[k].X;
    origin[k] = XOs[k].origin;
  }
  free(XOs);

  double *ranks = assign_ranks(X, NULL, N);

//   for (int k = 0; k < (n1+n2); k++)
//     printf("[%3d] rank = %6.2f  data = %" PRId64 " from %d\n",
//  	   k, ranks[k], X[k], origin[k]);

  return (RankedCombinedSample) {n1, n2, X, origin, ranks};
}

RankedCombinedSample rank_difference_magnitude(Usage *usage,
					       int start1, int end1,
					       int start2, int end2,
					       FieldCode fc) {
  int n1 = end1 - start1;
  int n2 = end2 - start2;
  int N = n1 * n2;

  if ((n1 <= 0) || (n2 <= 0))
    PANIC("Invalid sample sizes");
  if (((start1 >= start2) && (start1 < end2))
      || ((end1 > start2) && (end1 < end2)))
    PANIC("Invalid sample index ranges in usage structure: "
	  "[%d, %d) and [%d, %d)", start1, end1, start2, end2);

  int64_t *X = malloc(N * sizeof(int64_t));
  if (!X) PANIC_OOM();
  int *signs = malloc(N * sizeof(int));
  if (!signs) PANIC_OOM();

  for (int i = 0; i < n1; i++) 
    for (int j = 0; j < n2; j++) {
      int64_t diff = get_int64(usage, start1 + i, fc) - get_int64(usage, start2 + j, fc);
      int idx = i*n2+j;
      signs[idx] = (diff < 0) ? -1 : ((diff > 0) ? 1 : 0);
      diff *= signs[idx];
      //printf("i=%d, j=%d, idx=%d, diff = %lld\n", i, j, idx, diff);
      X[idx] = diff;
    }

//   for (int k = 0; k < N; k++)
//     printf("[%3d] data = %" PRId64 "\n", k, X[k]);

  int *index = malloc(N * sizeof(int));
  if (!index) PANIC_OOM();
  for (int i = 0; i < N; i++) index[i] = i;
  sort(index, N, sizeof(int), i64_lt, X);

//   for (int k = 0; k < N; k++)
//     printf("[%3d] sorted data = %" PRId64 "\n", k, X[index[k]]);

  double *ranks = assign_ranks(X, index, N);

  // Restore signs
  for (int k = 0; k < N; k++)
    X[k] *= signs[k];
  
  // Eliminate index by using it to reorder X
  int64_t *Y = malloc(N * sizeof(int64_t));
  if (!Y) PANIC_OOM();
  for (int k = 0; k < N; k++)
    Y[k] = X[index[k]];
  
//   for (int k = 0; k < N; k++)
//     printf("[k = %3d] rank %3.1f, data = %" PRId64 "\n",
// 	   k, ranks[k], Y[k]);

  free(X);
  free(index);
  return (RankedCombinedSample) {n1, n2, Y, NULL, ranks};
}

RankedCombinedSample rank_difference_signed(Usage *usage,
					    int start1, int end1,
					    int start2, int end2,
					    FieldCode fc) {
  int n1 = end1 - start1;
  int n2 = end2 - start2;
  int N = n1 * n2;

  if ((n1 <= 0) || (n2 <= 0))
    PANIC("Invalid sample sizes");
  if (((start1 >= start2) && (start1 < end2))
      || ((end1 > start2) && (end1 < end2)))
    PANIC("Invalid sample index ranges in usage structure");

  int64_t *X = malloc(N * sizeof(int64_t));
  if (!X) PANIC_OOM();

  for (int i = 0; i < n1; i++) 
    for (int j = 0; j < n2; j++) {
      int64_t diff = get_int64(usage, start1 + i, fc) - get_int64(usage, start2 + j, fc);
      int idx = i*n2+j;
      X[idx] = diff;
    }

//   for (int k = 0; k < N; k++)
//     printf("[%3d] data = %" PRId64 "\n", k, X[k]);

  sort(X, N, sizeof(int64_t), i64_lt, NULL);

  double *ranks = assign_ranks(X, NULL, N);

//   for (int k = 0; k < N; k++)
//     printf("[%3d] rank %6.1f, sorted data = %" PRId64 "\n", k, ranks[k], X[k]);

  return (RankedCombinedSample) {n1, n2, X, NULL, ranks};
}

// RCS must rank the set of n1 * n2 sample DIFFERENCES by their
// MAGNITUDE.  
double mann_whitney_w(RankedCombinedSample RCS) {
  int N = RCS.n1 * RCS.n2;
  int count_zero = 0;
  int count_pos = 0;
  for (int i = 0; i < N; i++) {
    if (RCS.X[i] == 0) count_zero++;
    else if (RCS.X[i] > 0) count_pos++;
  }
  double W = count_pos;
  W += 0.5 * (double) count_zero;
  W += 0.5 * (double) (RCS.n1 * (RCS.n1 + 1));
  return W;
}

// RCS must rank the combined n1 + n2 samples and the RCS 'index'
// array is being used here to indicate the observation's origin: 1
// for sample 1, and 2 for sample 2.
//
// Also called the Wilcoxon rank sum.  And sometimes labeled W.
//
double mann_whitney_u(RankedCombinedSample RCS, double *U1, double *U2) {
  double R1 = 0.0;
  double R2 = 0.0;
  int N = RCS.n1 + RCS.n2;
  for (int i = 0; i < N; i++) {
    if (RCS.index[i] == 1)
      R1 += RCS.rank[i];
    else
      R2 += RCS.rank[i];
  }
  *U1 = R1 - (double) (RCS.n1 * (RCS.n1 + 1)) / 2.0;
  *U2 = R2 - (double) (RCS.n2 * (RCS.n2 + 1)) / 2.0;
  
  if ((*U1 + *U2) != (double) (RCS.n1 * RCS.n2))
    printf("ERROR!  Mann-Whitney U test inconsistent.\n");

  return (*U1 < *U2) ? *U1 : *U2;
  
}

#if 0
// Requires ranked combined sample of n1 + n2 values
double mann_whitney_p(RankedCombinedSample RCS, double U) {
  int n1 = RCS.n1;
  int n2 = RCS.n2;
  int N = n1 + n2;
  // Mean and std dev for U assumes U is normally distributed, which
  // it will be according to the theory.
  double meanU = (double)(n1 * n2) / 2.0;
  double mean_dist = U - meanU;
  double stddev = sqrt((double) (n1 * n2) / (double) (N * (N - 1)));
  double correction = sqrt((double)(N * N * N - N) / 12.0 - tie_correction(RCS));
  stddev = stddev * correction;
  double Z = mean_dist / stddev;
  // p-value for hypothesis that median 1 < median 2
  return normalCDF(Z);
}
#endif

// Requires ranked combined sample: n1 + n2 values
static double tie_correction(RankedCombinedSample RCS) {
  int N = RCS.n1 + RCS.n2;
  // tc is the number of ranks that have ties
  // counts[i] = number of ties for the ith tied rank
  int tc = 0;
  int *counts = calloc(N, sizeof(int));
  if (!counts) PANIC_OOM();
  // TODO: Remove counts[] after debugging
  double t3term, correction = 0.0;
  bool counting = false;
  for (int k = 0; k < N; k++) {
    if (k > 0) {
      if (RCS.rank[k] == RCS.rank[k-1]) {
	if (counting) {
	  counts[tc]++;
	} else {
	  counting = true;
	  counts[tc] += 1 + (counts[tc] == 0);
	}
      } else if (counting) {
	counting = false;
	t3term = (double) (counts[tc] * counts[tc] * counts[tc] - counts[tc]);
	correction +=  t3term;
	//printf("%d ranks tied at rank %.1f\n", counts[tc], RCS.rank[k-1]);
	tc++;
      }
    }
  }
  //printf("Number of ranks with ties = %d\n", tc);
//   int total_ties = 0;
//   for (int k = 0; k < tc; k++) {
//     total_ties += counts[k];
    //printf("kth rank %3d, ties = %d\n", k, counts[k]);
  //  }
  //printf("Summary: tie count = %d, total ties = %d, N = %d\n", tc, total_ties, N);

//     printf("Combined rank [%3d] rank %3.1f, data = %" PRId64 "\n",
// 	   k, RCS.rank[k], X[k]);

  //printf("Tie correction (sum term only) = %8.2f\n", correction);
  return correction;
}

// This version works with W, not U, and agrees with the results
// shown, e.g. at 
// https://www.statsdirect.co.uk/help/nonparametric_methods/mann_whitney.htm
// https://statisticsbyjim.com/hypothesis-testing/mann-whitney-u-test/
// https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/mann-whitney-test/methods-and-formulas/methods-and-formulas/
//
double mann_whitney_p(RankedCombinedSample RCS, double W, double *adjustedp) {
  int n1 = RCS.n1;
  int n2 = RCS.n2;
  double K = fmin(W, n1 * (n1 + n2 + 1) - W);
  //printf("K = %8.3f\n", K);
  // Mean and std dev for W assumes W is normally distributed, which
  // it will be according to the theory
  double meanW = 0.5 * (double)(n1 * (n1 + n2 + 1));
  // Continuity correction is 0.5, accounting for the fact that our
  // samples are not real numbered values of a function (which would
  // never produce a tie).
  double cc = 0.5;
  double mean_distance = fabs(W - meanW);
  double mean_distanceK = fabs(K - meanW);
  double stddev = sqrt((double) (n1 * n2 * (n1 + n2 + 1)) / 12.0);
   // p-value for hypothesis that η₁ ≠ η₂
  double Zne = (mean_distanceK - cc) / stddev;
  double p = 2 * (1 - normalCDF(Zne));
  //printf("Unadjusted p value η₁ ≠ η₂ is %.4f\n", p);
  //double Zne = (k + cc) / stddev;
  //printf("Unadjusted p value η₁ ≠ η₂ is %.4f\n", 2 * (1 - normalCDF(Zne)));


  double f1 = (double) (n1 * n2) / (double) ((n1 + n2)*(n1 + n2 - 1));
  double addend1 = pow((double) (n1 + n2), 3.0) / 12.0;
  double addend2 = tie_correction(RCS) / ((n1 + n2)*(n1+n2-1));
  double stddev_adjusted = sqrt(f1) * sqrt(addend1 - addend2);
  //printf("STDDEV = %.4f, ADJUSTED = %.4f\n", stddev, stddev_adjusted);
  double adjustedZ = (mean_distanceK - cc) / stddev_adjusted;
  //printf("Zne = %.4f, ADJUSTED Zne = %.4f\n", Zne, adjustedZ);
  if (adjustedp) {
    *adjustedp = 2 * (1 - normalCDF(adjustedZ));
    //printf("Adjusted for ties, p value is %.4f\n", *adjustedp);
  }

  return p;
}

// https://www.statsdirect.co.uk/help/nonparametric_methods/mann_whitney.htm
// "When samples are large (either sample > 80 or both samples >30) a
// normal approximation is used for the hypothesis test and for the
// confidence interval."
//
// Returns low end of confidence interval, sets '*highptr' to high end.
// 
int64_t mann_whitney_U_ci(RankedCombinedSample RCS, double alpha, int64_t *highptr) {
  int n1 = RCS.n1;
  int n2 = RCS.n2;
  int N = n1 * n2;
  double Zcrit = inverseCDF(1 - alpha/2.0);
  double meanU = (double) N / 2.0;
  double U1, U2;
  double U = mann_whitney_u(RCS, &U1, &U2);
  double stddev = sqrt((double) (n1 * n2 * (n1 + n2 + 1)) / 12.0);
  //double stderror = stddev / sqrt(N);
  //double margin = Zcrit * stderror;
  double margin = Zcrit * stddev;
  printf("U = %.4f, Zcrit = %.4f, est. meanU = %.3f, stddev = %.3f, margin = %.3f\n",
	 U, Zcrit, meanU, stddev, margin);
  double low = U - margin;
  double high = U + margin;
  printf("  Margin = %.3f, low = %.3f, high = %.3f\n", margin, low, high);
  printf("                 low = %.3f, high = %.3f\n", low/(double)N, high/(double)N);
  printf("  Z_u = %.3f\n", (U - meanU) / stddev);
  printf("\n");
  double f1 = (double) (n1 * n2) / (double) ((n1 + n2)*(n1 + n2 - 1));
  double addend1 = pow((double) (n1 + n2), 3.0) / 12.0;
  double addend2 = tie_correction(RCS) / 12.0;
  double stddev_adjusted = sqrt(f1) * sqrt(addend1 - addend2);
  printf("stddev = %.4f, ADJUSTED stddev = %.4f\n", stddev, stddev_adjusted);
  //double margin_adjusted = Zcrit * stddev_adjusted;
  double margin_adjusted = Zcrit * stddev_adjusted;
  printf("margin = %.4f, ADJUSTED margin = %.4f\n", margin, margin_adjusted);
  low = U - margin_adjusted;
  high = U + margin_adjusted;
  printf("  Adjusted = %.3f, low = %.3f, high = %.3f\n", margin_adjusted, low, high);
  printf("                   low = %.3f, high = %.3f\n", low/(double)N, high/(double)N);
  double Zu = (U - meanU - 0.5) / stddev_adjusted;
  printf("  Z_u = %.3f\n", Zu);
  printf("  corresponding p =  %.4f\n", normalCDF(Zu));
  printf("\n");
// Calculate ranks for each end of confidence interval
//   int lowidx = -1, highidx = -1;
//   for (int k = 0; k < N; k++) {
//     int rank = RCS.rank[k];
//     if (rank <= low) lowidx = k;
//     if ((highidx == -1) && (rank >= high)) highidx = k;
//   }
//   if ((lowidx == -1) || (highidx == -1)) {
//     printf("ERROR -- lowidx = %d, highidx = %d\n", lowidx, highidx);
//     PANIC("");
//   }
//   printf("Low rank [%d] = %.1f, X = %lld\n", lowidx, RCS.rank[lowidx], RCS.X[lowidx]);
//   printf("High rank [%d] = %.1f, X = %lld\n", highidx, RCS.rank[highidx], RCS.X[highidx]);
//   *highptr = RCS.X[highidx];
//   return RCS.X[lowidx];
  *highptr = high;
  return low;
}

// https://www.statsdirect.co.uk/help/nonparametric_methods/mann_whitney.htm
// "When samples are large (either sample > 80 or both samples >30) a
// normal approximation is used for the hypothesis test and for the
// confidence interval."
//
// https://seismo.berkeley.edu/~kirchner/eps_120/Toolkits/Toolkit_08.pdf
// "If m and n are small (say, mn<100 or so), then other methods are
// necessary to estimate confidence limits.  See Helsel and Hirsch."
//
// Input: RCS contains signed ranked differences.
// Returns confidence level, e.g. 0.955 for 95.5%, and sets
// 'lowptr' and 'highptr' to the ends of the interval.
// 
// Note: The calculation below agress with minitab and whatever system
// statsdirect uses.  The CI differs a bit from that of "Statistics by
// Jim" as he reports a narrower range by 1 rank on the low side and 1
// rank on the high side.  Which approach is best?
//
double median_diff_ci(RankedCombinedSample RCS,
		      double alpha,
		      int64_t *lowptr,
		      int64_t *highptr) {
  double n1 = RCS.n1;
  double n2 = RCS.n2;
  double N = n1 * n2;
  double Zcrit = inverseCDF(1.0 - alpha/2.0);
  // Calculate ranks for each end of confidence interval
  double low = (N / 2.0) - Zcrit * sqrt(N * (n1 + n2 + 1) / 12.0);
  low = floor(low);
  double high = floor(N - low + 1);
  int lowidx = -1, highidx = -1;
  for (int k = 0; k < N; k++) {
    int rank = RCS.rank[k];
    if ((lowidx == -1) && (rank > low)) lowidx = k;
    if (rank < high) highidx = k;
  }
  if (lowidx == -1) lowidx = 0;
  if (highidx == -1) highidx = N - 1;
  *lowptr = RCS.X[lowidx];
  *highptr = RCS.X[highidx];

  double ci_width = highidx - lowidx;
  double actual_Z = ci_width / 2.0 / sqrt(N * (n1 + n2 + 1) / 12.0);
  double confidence =  2.0 * normalCDF(actual_Z) - 1.0;
  return confidence;
}

double ranked_diff_Ahat(RankedCombinedSample RCS) {
  double n1 = RCS.n1;
  double n2 = RCS.n2;
  int N = RCS.n1 * RCS.n2;
  int count_zero = 0;
  int count_pos = 0;
  for (int k = 0; k < N; k++) {
    //printf("[%2d] rank = %.1f, X = %lld\n", k, RCS.rank[k], RCS.X[k]);
    if (RCS.X[k] == 0) count_zero++;
    else if (RCS.X[k] > 0) count_pos++;
  }
  double sum = (double) count_pos + 0.5 * (double) count_zero;
  double R1 = (n1 * (n1+1) / 2.0) + sum;
  double Ahat = (R1/n1 - (n1+1)/2.0) / n2;
  return Ahat;
}

// Hodges-Lehmann estimation of location shift, sometimes called the
// median difference.
double median_diff_estimate(RankedCombinedSample RCS) {
  int N = RCS.n1 * RCS.n2;
  if (N < 1) PANIC("Invalid ranked combined sample");
  int h = N / 2;
  int hminus1 = h - 1;
  if (RCS.index) {
    h = RCS.index[h];
    hminus1 = RCS.index[hminus1];
  }
  if (2 * h == N) 
    return (RCS.X[h-1] + RCS.X[h]) / 2.0;
  return RCS.X[h];
}

// -----------------------------------------------------------------------------
// Misc
// -----------------------------------------------------------------------------

static int compare_median_total_time(const void *idx_ptr1,
			      const void *idx_ptr2,
			      void *context) {
  Summary **s = context;
  const int idx1 = *((const int *)idx_ptr1);
  const int idx2 = *((const int *)idx_ptr2);
  int64_t val1 = s[idx1]->total.median;
  int64_t val2 = s[idx2]->total.median;
  if (val1 > val2) return 1;
  if (val1 < val2) return -1;
  return 0;
}

int *sort_by_totaltime(Summary *summaries[], int start, int end) {
  if (!summaries) PANIC_NULL();
  int n = end - start;
  if (n < 1) return NULL;
  int *index = malloc(n * sizeof(int));
  if (!index) PANIC_OOM();
  for (int i = 0; i < (end - start); i++) index[i] = i+start;
  sort(index, n, sizeof(int), compare_median_total_time, summaries);
  return index;
}
