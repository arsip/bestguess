//  -*- Mode: C; -*-                                                       
// 
//  cdf.c
// 
//  (C) Jamie A. Jennings, 2024

// Phi and cPhi functions are published here:
//
//   Marsaglia, G. (2004). Evaluating the Normal Distribution.
//   Journal of Statistical Software, 11(4), 1–11.
//   https://doi.org/10.18637/jss.v011.i04
//
// All other code here is my own.

#include <math.h>
#include <stdio.h>

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

double approx_one_minus_p(double Z) {
  const long double sqrtpibytwo = sqrtl(M_PI_2);
  long double ZZ = Z;
  return expl(-(ZZ*ZZ)/2.0) / (ZZ * sqrtpibytwo);
}

// The CDF of the normal function increases monotonically, so we can
// numerically invert it using binary search.  It's not fast, but we
// don't need it to be fast.
double invPhi(double p) {
  if (p <= 0.0) return -8.0;
  if (p >= 1.0) return  8.0;
  double Zhigh = (p < 0.5) ? 0.0 : 8.0;
  double Zlow = (p < 0.5) ? -8.0 : 0.0;
  double Zmid, approx, err;
  while (1) {
    //printf("    p = %5.3f  Zhigh = %5.3f  Zlow = %5.3f\n", p, Zhigh, Zlow);
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

int main(int argc, char **argv) {
  double Z;
#if 0
  printf("  Z        Phi           cPhi\n");
  for (int i = -500; i < 501; i++) {
    Z = (double) i / 100.0;
    printf("%5.2f  %12.9f  %12.9f\n", Z, Phi(Z), cPhi(Z));
  }
#endif

  Z = -5.00;
  while (Phi(Z) > 0.0) Z -= 0.10;
  printf("Phi(%f) = 0.0\n", Z);

  Z = 5.00;
  while (Phi(Z) < 1.0) Z += 0.10;
  printf("Phi(%f) = 1.0\n", Z);


  // R studio:
  //   install.packages("Rmpfr")
  //   library(Rmpfr)
  //   v=mpfr(1:10,120);
  //   pr=1-pnorm(v);
  //   cbind(v,pr);
  // These are the single-tail areas.  Double them to get what we want.
  long double published[] =
    { 0,					   // spacer
      0.15865525393145705141476745436796207774,    // Z = 1
      0.022750131948179207200282637166533437700,   // Z = 2
      0.0013498980316300945266518147675949769914,  // Z = 3
      3.1671241833119921253770756722151496426e-5,  // Z = 4
      2.8665157187919391167375233287471617709e-7,  // Z = 5
      9.8658764503769814070086413267918909381e-10, // Z = 6
      1.2798125438858350043836233949366514843e-12, // Z = 7
      6.2209605742717841235169694954712541828e-16, // Z = 8
      1.1285884059538406473237309503891605935e-19, // Z = 9
      7.6198530241601301905642134646022885746e-24  // Z = 10
    };

#if 0
  // https://en.wikipedia.org/wiki/Normal_distribution
  double published[] =
    {0,				// spacer
     0.317310507863,		// Z = 1
     0.045500263896, 		// Z = 2
     0.002699796063,		// Z = 3
     0.000063342484,		// Z = 4
     0.000000573303,		// Z = 5
     0.000000001973};		// Z = 6
#endif

  double phi, cphi, p, delta_calc, delta_approx;
  long double published_val, one_in_n;
  int published_n = sizeof(published) / sizeof(long double);

  printf("   Z      PUBLISHED 1-p        Calc. 1-p              (Δ)            Approximation         (Δ)         Winner\n");
  for (int i = 0; i < 1001; i += 100) {
    Z = (double) i / 100.0;
    phi = Phi(Z);
    cphi = cPhi(Z);
    p = phi - cphi;
    published_val = (((i/100) > 0) && ((i/100) < published_n)) ? (2.0 * published[i/100]) : 0;
    delta_calc = 1.0 - p - published_val;
    delta_approx = approx_one_minus_p(Z) - published_val;
    printf("%5.2f %22.20Lf %18.16f %17.14f %18.16f %17.14f %s\n",
	   Z, published_val, 1.0-p, delta_calc, approx_one_minus_p(Z), delta_approx,
	   (fabs(delta_calc) < fabs(delta_approx)) ? "Calc." : "Approx.");
  }

  printf("\n");
  printf("   Z           p                   1-p               1 / (1-p)\n");
  for (int i = 100; i < 701; i += 100) {
    Z = (double) i / 100.0;
    phi = Phi(Z);
    cphi = cPhi(Z);
    p = phi - cphi;
    one_in_n = 1.0L / (1.0L - (long double) p);
    printf("%5.2f %18.16f  %18.16f  %18.1Lf\n", Z, p, 1.0-p, one_in_n);
  }

  // The code immediately above produces this table:
  //
  //    Z           p                   1-p               1 / (1-p)
  //  1.00 0.6826894921370859  0.3173105078629141                 3.2
  //  2.00 0.9544997361036414  0.0455002638963586                22.0
  //  3.00 0.9973002039367395  0.0026997960632605               370.4
  //  4.00 0.9999366575163340  0.0000633424836660             15787.2
  //  5.00 0.9999994266968566  0.0000005733031434           1744277.9
  //  6.00 0.9999999980268238  0.0000000019731762         506797117.8
  //  7.00 0.9999999999974398  0.0000000000025602      390598406536.9
  //
  // Our calculated values are agree with the probabilities at
  // https://en.wikipedia.org/wiki/68–95–99.7_rule up to n=5 (5 sigma
  // away from the mean).  At n=6, the 6 most significant digits
  // agree, and at n=7, the 3 most significant digits agree.
  //
  // Note: A similar table is found at
  // https://en.wikipedia.org/wiki/Standard_deviation#Rules_for_normally_distributed_data

  double pval;
  printf("\n");
  for (int p = 0; p < 1000; p += 1) {
    pval = (double) p / 1000.0;
    Z = invPhi(pval);
    printf("p = %6.4f --> Z-score = %6.3f --> verified at %7.5f  Δ = %9.6f\n",
	   pval, Z, Phi(Z), pval - Phi(Z));
  }

}
