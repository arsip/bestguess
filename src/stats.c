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

   However, if we are more concerned with the long tail, then the 95th
   and 99th percentile values should be highlighted.

   Given a choice between either median or mean, the median of a
   right-skewed distribution is typically the closer of the two to the
   mode.

 */

// -----------------------------------------------------------------------------
// Z score calculation without a table
// -----------------------------------------------------------------------------

// Cumulative (less than Z) Z-score calculation.  Instead of a table,
// we iteratively estimate each needed value to an accuracy greater
// than that of most tables.
//
// Benefits:
//
// (1) Tables are only accurate to 5 digits, and we can get
// around 15 digits from Phi(x) below.
//
// (2) Tables usually provide values for Z-scores from about -4.0 to
// 4.0, and we can get fairly accurate results out to -8.0 and 8.0.
//
// Phi and cPhi functions are published here:
//
//   Marsaglia, G. (2004). Evaluating the Normal Distribution.
//   Journal of Statistical Software, 11(4), 1–11.
//   https://doi.org/10.18637/jss.v011.i04
//
// Phi(x) produces "an absolute error less than 8×10^−16."  Marsaglia
// suggests returning 0 for x < −8 and 1 for x > 8 "since an error of
// 10^−16 can make a true result near 0 negative, or near 1, exceed 1."
//
static double Phi(double x) {
  if (x < -8.0) return 0.0;
  if (x > 8.0) return 1.0;
  long double s = x, t = 0.0, b = x, q = x*x, i = 1.0;
  while (s != t)
    s = (t=s) + (b *= q / (i += 2));
  return 0.5 + s * exp(-0.5 * q - 0.91893853320467274178L);
}

static double cPhi(double x) {
  int j= 0.5 * (fabs(x) + 1.0);
  long double R[9]= { 1.25331413731550025L,
		      0.421369229288054473L,
		      0.236652382913560671L,
		      0.162377660896867462L,
		      0.123131963257932296L,
		      0.0990285964717319214L,
		      0.0827662865013691773L,
		      0.0710695805388521071L,
		      0.0622586659950261958L };
  long double pwr = 1.0, a = R[j], z = 2*j, b = a*z - 1;
  long double h = fabs(x) - z, s = a + h*b, t = a, q = h * h;
  for (int i = 2; s != t; i += 2){
    a = (a + z*b)/i;
    b = (b+ z*a)/(i+1);
    pwr *= q;
    s = (t=s) + pwr*(a + h*b);
  }
  s = s * exp(-0.5*x*x - 0.91893853320467274178L);
  if (x >= 0)
    return (double) s;
  return (double) (1.0 - s);
}

// The CDF of the normal function increases monotonically, so we can
// numerically invert it using binary search.  It's not fast, but we
// don't need it to be fast.
static double invPhi(double p) {
  if (p <= 0.0) return -8.0;
  if (p >= 1.0) return  8.0;
  double Zhigh = (p < 0.5) ? 0.0 : 8.0;
  double Zlow = (p < 0.5) ? -8.0 : 0.0;
  double Zmid, approx, err;
  while (1) {
    Zmid = Zlow + (Zhigh - Zlow) / 2.0;
    approx = Phi(Zmid);
    err = approx - p;
    if (fabs(err) < 0.000001)	// Adjust as necessary
      return Zmid;
    if (err > 0.0) 
      Zhigh = Zmid;
    else
      Zlow = Zmid;
  }
}

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
  if (AD <= 0.20) return 1.0 - exp(-13.436 + 101.14*AD - 223.73*AD*AD);
  if (AD <= 0.34) return 1.0 - exp(-8.318 + 42.796*AD - 59.938*AD*AD);
  if (AD <  0.60) return exp(0.9177 - 4.279*AD - 1.38*AD*AD);
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

// Returns true if we were able to calculate an AD score, and false if
// we encountered Z scores too high to do the calculation.  In the
// latter case, the 'ADscore' value is the most extreme Z score seen.
// The returned boolean should be saved as CODE_HIGHZ so that the
// meaning of 'ADscore' can be properly interpreted.
//
static bool AD_normality(int64_t *X,    // ranked data points
			 int n,         // number of data points
			 double mean,
			 double stddev,
			 double *ADscore) {
  assert(X && (n > 0));
  if (!X || !ADscore) PANIC_NULL();

  double *Y = malloc(n * sizeof(double));
  if (!Y) PANIC_OOM();

  // Set zero value for 'extremeZ' as a sentinel
  double extremeZ = 0.0;

  for (int i = 0; i < n; i++) {
    // Y is a vector of studentized residuals
    Y[i] = ((double) X[i] - mean) / stddev;
    if (Y[i] != Y[i]) {
      PANIC("Got NaN.  Should not have attempted AD test because stddev is zero.");
    }
    // Extreme values (i.e. high Z scores) indicate a long tail, and
    // prevent the computation of a meaningful AD score.  NOTE: An
    // observation that is around 7 std deviations from the mean will
    // occur in a normal distribution with probability 1 in 390 BILLION.
    // See e.g. https://en.wikipedia.org/wiki/68–95–99.7_rule
    if (fabs(Y[i]) > 7.0) {
      // Track the highest Z score we encounter
      if (fabs(Y[i]) > fabs(extremeZ))
	extremeZ = Y[i];
    }
  }
  *ADscore = (extremeZ != 0.0) ? extremeZ : AD_from_Y(n, Y, Phi);
  free(Y);
  return (extremeZ == 0.0);
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

// When the AD test for normality says "not close to normal", we may
// want to know why.  The kurtosis can tell us if the shape is not
// normal in the sense of the peak and tails, and skewness can tell us
// about asymmetry.

// https://www.stat.cmu.edu/~hseltman/files/spssSkewKurtosis.R
// spssSkewKurtosis=function(x) {
//   w=length(x)
//   m1=mean(x)
//   m2=sum((x-m1)^2)
//   m3=sum((x-m1)^3)
//   m4=sum((x-m1)^4)
//   s1=sd(x)
//   skew=w*m3/(w-1)/(w-2)/s1^3
//   sdskew=sqrt( 6*w*(w-1) / ((w-2)*(w+1)*(w+3)) )
//   kurtosis=(w*(w+1)*m4 - 3*m2^2*(w-1)) / ((w-1)*(w-2)*(w-3)*s1^4)
//   sdkurtosis=sqrt( 4*(w^2-1) * sdskew^2 / ((w-3)*(w+5)) )
//   mat=matrix(c(skew,kurtosis, sdskew,sdkurtosis), 2,
//         dimnames=list(c("skew","kurtosis"), c("estimate","se")))
//   return(mat)
// }

// Excess kurtosis = (1 / n) * Σ((X_i - mean) / std)⁴ - 3
//   > 0 ==> "sharp peak, heavy tails"
//   < 0 ==> "flat peak, light tails"
//   near zero, the distribution resembles a normal one
static double kurtosis(int64_t *X,
		       int n,
		       double mean,
		       double stddev) {
  double sum = 0.0;
  for (int i = 0; i < n; i++)
    sum += pow((X[i] - mean) / stddev, 4.0);
  return (sum / n) - 3.0;
}

// Moment-based calculation of skew
// skew = (n / ((n - 1) * (n - 2))) * Σ((X_i - mean) / std)³
//   abs(skew) < 0.5 ==> approximately symmetric
//   0.5 < abs(skew) < 1.0 ==> moderately skewed
//   abs(skew) > 1.0 ==> highly skewed
static double skew(int64_t *X,
		   int n,
		   double mean,
		   double stddev) {
  double sum = 0.0;
  for (int i = 0; i < n; i++)
    sum += pow((X[i] - mean) / stddev, 3.0);
  return sum * n / (n-1) / (n-2);
}

// "As the standard errors get smaller when the sample size increases,
// z-tests under null hypothesis of normal distribution tend to be
// easily rejected in large samples"
// https://www.ncbi.nlm.nih.gov/pmc/articles/PMC3591587/
//
// If we follow the guidance in that article, we should use absolute
// values of 2 (skew) and 4 (excess kurtosis) as thresholds of
// substantial deviation from normality for sample sizes > 300.
// 
// Sample sizes < 50
//   ==> Reject normality when Zskew or Zkurtosis > 1.96 (alpha = .05)
// Sample sizes 50..300
//   ==> Reject normality when Zskew or Zkurtosis > 3.29 (alpha = .001)
//
// Similar advice is given in
// https://www.ncbi.nlm.nih.gov/pmc/articles/PMC3693611/
//
// Reminder:
//   α = .05 ==> critical Z score (abs) 1.96
//   α = .01 ==> critical Z score (abs) 2.58
//   α = .001 ==> critical Z score (abs) 3.29

static double Zcrit(double alpha) {
  return fabs(invPhi(alpha / 2.0));
}

static double skew_stddev(int n) {
  return sqrt(6.0 * n * (n-1) / ((n-2) * (n+1) * (n+3)));
}

static double skew_kurtosis_Zcrit(int n) {
  if (n > 100) return Zcrit(0.001);
  if (n > 50) return Zcrit(0.01);
  return Zcrit(0.05);
}

static bool nonnormal_skew(double skew, int n) {
  static double skew_large_sample = 2.0;
  if (n > 300) return (fabs(skew) > skew_large_sample);
  double sdskew = skew_stddev(n);
  double Zabs = fabs(skew / sdskew);
  return (Zabs > skew_kurtosis_Zcrit(n));
}

static bool nonnormal_kurtosis(double kurtosis, int n) {
  if (n > 300) return (fabs(kurtosis) > 4.0); // 7.0 - 3.0
  double sdskew = skew_stddev(n);
  double sdkurtosis = sqrt(4.0 * (n*n - 1) * sdskew * sdskew / ((n-3)*(n+5)));
  double Zabs = fabs(kurtosis / sdkurtosis);
  return (Zabs > skew_kurtosis_Zcrit(n));
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

  m->skew = skew(X, runs, m->est_mean, m->est_stddev);
  if (nonnormal_skew(m->skew, runs))
    SET(m->code, CODE_HIGH_SKEW);
  
  m->kurtosis = kurtosis(X, runs, m->est_mean, m->est_stddev);
  if (nonnormal_kurtosis(m->kurtosis, runs))
    SET(m->code, CODE_HIGH_KURTOSIS);

  // Off-the-charts z-scores are detected when calculating the AD score.
  if (!AD_normality(X, runs, m->est_mean, m->est_stddev, &(m->ADscore))) {
    SET(m->code, CODE_HIGHZ);
    goto noscore;
  }
  m->p_normal = calculate_p(m->ADscore);
  free(X);
  return;

 noscore:
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

static RankedCombinedSample rank_difference_magnitude(Usage *usage,
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

static RankedCombinedSample rank_difference_signed(Usage *usage,
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

  sort(X, N, sizeof(int64_t), i64_lt, NULL);
  double *ranks = assign_ranks(X, NULL, N);
  return (RankedCombinedSample) {n1, n2, X, NULL, ranks};
}

// RCS must rank the set of n1 * n2 sample DIFFERENCES by their
// MAGNITUDE.  
static double mann_whitney_w(RankedCombinedSample RCS) {
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

static double tie_correction(RankedCombinedSample RCS) {
  int N = RCS.n1 + RCS.n2;
  // tc is the number of ranks that have ties
  // counts[i] = number of ties for the ith tied rank
  double t3term, correction = 0.0;
  bool counting = false;
  int count = 0;
  for (int k = 0; k < N; k++) {
    if (k > 0) {
      if (RCS.rank[k] == RCS.rank[k-1]) {
	if (counting) {
	  count++;
	} else {
	  counting = true;
	  count += 1 + (count == 0);
	}
      } else if (counting) {
	counting = false;
	t3term = (double) (count * count * count - count);
	correction +=  t3term;
	count = 0;
      }
    }
  }
  return correction;
}

// This version works with W, not U, and agrees with others' results, e.g.
// 
// https://www.statsdirect.co.uk/help/nonparametric_methods/mann_whitney.htm
// https://statisticsbyjim.com/hypothesis-testing/mann-whitney-u-test/
// https://support.minitab.com/en-us/minitab/help-and-how-to/statistics/nonparametrics/how-to/mann-whitney-test/methods-and-formulas/methods-and-formulas/
//
static double mann_whitney_p(RankedCombinedSample RCS, double W, double *adjustedp) {
  int n1 = RCS.n1;
  int n2 = RCS.n2;
  double K = fmin(W, n1 * (n1 + n2 + 1) - W);
  // Mean and std dev for W assumes W is normally distributed, which
  // it will be according to the theory
  double meanW = 0.5 * (double)(n1 * (n1 + n2 + 1));
  // Continuity correction is 0.5, accounting for the fact that our
  // samples are not real numbered values of a function (which would
  // never produce a tie).
  double cc = 0.5;
  double mean_distanceK = fabs(K - meanW);
  double stddev = sqrt((double) (n1 * n2 * (n1 + n2 + 1)) / 12.0);
   // p-value for hypothesis that η₁ ≠ η₂
  double Zne = (mean_distanceK - cc) / stddev;
  double p = 2 * cPhi(Zne);
  // At high Z scores, the calculation of p may produce a value
  // outside of [0.0, 1.0].  Here we clamp p to that range:
  if (p < 0.0) p = 0.0;
  if (p > 1.0) p = 1.0;
  if (adjustedp) {
    double f1 = (double) (n1*n2) / (double) ((n1+n2)*(n1+n2-1));
    double addend1 = pow((double) (n1+n2), 3.0) / 12.0;
    double addend2 = tie_correction(RCS) / ((n1+n2)*(n1+n2-1));
    double stddev_adjusted = sqrt(f1) * sqrt(addend1 - addend2);
    double adjustedZ = (mean_distanceK - cc) / stddev_adjusted;
    *adjustedp = 2 * cPhi(adjustedZ);
    if (*adjustedp < 0.0) *adjustedp = 0.0;
    if (*adjustedp > 1.0) *adjustedp = 1.0;
  }
  return p;
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
static double median_diff_ci(RankedCombinedSample RCS,
			     double alpha,
			     int64_t *lowptr,
			     int64_t *highptr) {
  double n1 = RCS.n1;
  double n2 = RCS.n2;
  double N = n1 * n2;
  double Zcrit = invPhi(1.0 - alpha/2.0);
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
  double confidence =  2.0 * Phi(actual_Z) - 1.0;
  return confidence;
}

static double ranked_diff_Ahat(RankedCombinedSample RCS) {
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
// median difference, but that term is easily confused with the
// difference of the medians.
// 
// https://en.wikipedia.org/wiki/Hodges–Lehmann_estimator "The
// Hodges–Lehmann statistic is the median of the m × n differences"
// between two samples.
//
static double median_diff_estimate(RankedCombinedSample RCS) {
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

// Caller must free the returned array
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


// 'idx' 0-based index into Summary array
Inference *compare_samples(Usage *usage,
			   int idx,
			   double alpha,
			   int ref_start, int ref_end,
			   int idx_start, int idx_end) {
  Inference *stat = malloc(sizeof(Inference));
  if (!stat) PANIC_OOM();
  stat->index = idx;

  RankedCombinedSample RCSmag =
    rank_difference_magnitude(usage,
			      ref_start, ref_end,
			      idx_start, idx_end,
			      F_TOTAL);
  stat->W = mann_whitney_w(RCSmag);
  stat->p = mann_whitney_p(RCSmag, stat->W, &(stat->p_adj));
  stat->p_super = ranked_diff_Ahat(RCSmag);

  RankedCombinedSample RCSsigned =
    rank_difference_signed(usage,
			   idx_start, idx_end,
			   ref_start, ref_end,
			   F_TOTAL);

  stat->shift = median_diff_estimate(RCSsigned);
  stat->confidence = median_diff_ci(RCSsigned,
				    alpha,
				    &(stat->ci_low),
				    &(stat->ci_high));

  stat->results = 0;

  // Is the p value (or adjusted one) not low enough?
  if (!(stat->p < alpha) || !(stat->p_adj < alpha))
    SET(stat->results, INF_NONSIG);

  // Check for end of CI interval being too close to zero
  bool ci_touches_0 = ((llabs(stat->ci_low) < config.ci_epsilon) ||
		       (llabs(stat->ci_high) < config.ci_epsilon));
  // Or CI outright includes zero
  bool ci_includes_0 = (stat->ci_low < 0) && (stat->ci_high > 0);

  if (ci_touches_0 || ci_includes_0)
    SET(stat->results, INF_CIZERO);

  // Check for median difference (effect size) too small
  if (fabs(stat->shift) < (double) config.min_effect) 
    SET(stat->results, INF_NOEFFECT);

  if (stat->p_super > config.high_superiority)
    SET(stat->results, INF_HIGHSUPER);
    
  free(RCSmag.X);
  free(RCSmag.rank);
  free(RCSsigned.X);
  free(RCSsigned.rank);
  return stat;
}
