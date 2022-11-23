#include <math.h>
#include <stdio.h>

#define TEST(M_) printf("%-24s%+2.13f\n", #M_, M_)
#define TESTI(M_) printf("%-24s%d\n", #M_, M_)

int
main(void)
{
	TEST(M_E);
	TEST(M_LOG2E);
	TEST(M_LOG10E);
	TEST(M_LN2);
	TEST(M_LN10);
	TEST(M_PI);
	TEST(M_PI_2);
	TEST(M_PI_4);
	TEST(M_1_PI);
	TEST(M_2_PI);
	TEST(M_2_SQRTPI);
	TEST(M_SQRT2);
	TEST(M_SQRT1_2);

	TEST(cos(M_PI_2));
	TEST(acos(cos(M_PI_2)));
	TEST(cosh(M_SQRT1_2));
	TEST(acosh(cosh(M_SQRT1_2)));
	TEST(sin(M_PI_2));
	TEST(asin(sin(M_PI_2)));
	TEST(sinh(M_PI_2));
	TEST(asinh(sinh(M_PI_2)));
	TEST(tan(M_SQRT2));
	TEST(atan(tan(M_SQRT2)));
	TEST(tanh(M_SQRT2));
	TEST(atanh(tanh(M_SQRT2)));
	TEST(atan2(M_SQRT2, M_SQRT2));
	TEST(cbrt(8.0));
	TEST(ceil(M_PI));
	TEST(ceil(M_E));
	TEST(copysign(-2.0, 10.0));
	TEST(copysign(2.0, -10.0));
	TEST(erf(M_SQRT1_2));
	TEST(erfc(M_SQRT1_2));
	TEST(exp(M_PI_4));
	TEST(exp2(3.0));
	TEST(expm1(M_PI_4));
	TEST(fabs(-M_SQRT2));
	TEST(fabs(M_SQRT2));
	TEST(fdim(M_PI, M_PI_2));
	TEST(fdim(M_PI, -M_PI_2));
	TESTI(finite(1.0 / 0.0));
	TEST(floor(M_PI));
	TEST(floor(M_E));
	TEST(fma(M_PI, M_E, M_SQRT2));
	TEST(fmax(M_PI, M_E));
	TEST(fmin(M_PI, M_E));
	TEST(fmod(100.5, 10.0));
	TEST(gamma(5.0));
	TEST(hypot(3.0, 4.0));
	printf("%-24s%d\n", "ilogb(10.0)", ilogb(10.0));
	TESTI(isinf(1.0 / 0.0));
	TESTI(isnan(log(-1.0)));
	TEST(j0(M_PI));
	TEST(j1(M_PI));
	TEST(jn(3, M_PI));
	TEST(lgamma(M_PI));
	TEST(log(M_E));
	TEST(log10(100.0));
	TEST(log1p(M_PI));
	TEST(nan(""));
#ifdef notyet
	// XXX: vax
	TEST(nextafter(1.0e-14, 1.0));
#endif
	TEST(pow(M_SQRT2, 2.0));
	TEST(remainder(M_PI, M_E));
	TEST(rint(M_PI));
	TEST(rint(M_E));
	printf("%-24s%+2.13f\n", "scalbn(1.0, 2)", scalbn(1.0, 2));
	TEST(sin(M_PI_4));
	TEST(sinh(M_PI_4));
	TEST(sqrt(9.0));
	TEST(tan(M_PI_4));
	TEST(tanh(M_PI_4));
	TEST(trunc(M_PI));
	TEST(trunc(M_E));
	TEST(y0(M_PI));
	TEST(y1(M_PI));
	TEST(yn(3, M_PI));

	return 0;
}
