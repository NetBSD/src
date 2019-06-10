#include <stdio.h>

#include <bi.h>

// use standard C definition to avoid .h
int test_exp_multi(void) {
	bi_t result;
	bi_t g[3];
	unsigned long e[3];

	bi_new( result);
	bi_new( g[0]);
	bi_new( g[1]);
	bi_new( g[2]);
	// result = (2^2 * 5^4 * 7^7) mod 56 -> should give 28
	bi_set_as_dec( g[0], "2");
	bi_set_as_dec( g[1], "5");
	bi_set_as_dec( g[2], "7");
	e[0] = 2L;
	e[1] = 4L;
	e[2] = 7L;
	bi_multi_mod_exp( result, 3, g, e, 56);
	printf("multi-exponentiation <result>=%s\n", bi_2_dec_char(result));
	bi_free( g[0]);
	bi_free( g[1]);
	bi_free( g[2]);
	bi_free( result);
	return 0;
}
