#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#define	 IM(x)	 ((intmax_t)(x))
#define	UIM(x)	((uintmax_t)(x))

#define TEST(id, a, b, c)                             			\
    if (b) {                            				\
	c = (a) / (b);                        				\
	printf(id "%16jx / %16jx => %16jx\n", UIM(a), UIM(b), UIM(c));  \
	c = (a) % (b);                        				\
	printf(id "%16jx / %16jx => %16jx\n", UIM(a), UIM(b), UIM(c));  \
    }
    

int main(int ac, char **av)
{
    int32_t sr32;
    uint32_t ur32;
    intmax_t sr64;
    uintmax_t ur64;
    int i, j, k, l;

    for(i = 0; i <= 31; ++i) {
	for(j = 0; j <= 31; ++j) {
	    for(k = -1; k <= 1; ++k) {
		for(l = -1; l <= 1; ++l) {
		    TEST("32 ",  (1 << i) + k,  (1 << j) + l, sr32);
		    TEST("32 ",  (1 << i) + k, -(1 << j) + l, sr32);
		    TEST("32 ", -(1 << i) + k,  (1 << j) + l, sr32);
		    TEST("32 ", -(1 << i) + k, -(1 << j) + l, sr32);
		    if (k < 0 && 1U << i < abs(k) ||
		        l < 0 && 1U << j < abs(l))
			    continue;
		    TEST("32U", (1U << i) + k, (1U << j) + l, ur32);
		}
	    }
	}
    }
    if (sizeof(intmax_t) == 4)
	exit(0);
    for(i = 0; i <= 63; ++i) {
	for(j = 0; j <= 63; ++j) {
	    for(k = -1; k <= 1; ++k) {
		for(l = -1; l <= 1; ++l) {
		    TEST("64 ",  (IM(1) << i) + k,  (IM(1) << j) + l, sr64);
		    TEST("64 ",  (IM(1) << i) + k, -(IM(1) << j) + l, sr64);
		    TEST("64 ", -(IM(1) << i) + k,  (IM(1) << j) + l, sr64);
		    TEST("64 ", -(IM(1) << i) + k, -(IM(1) << j) + l, sr64);
		    if (k < 0 && UIM(1U) << i < abs(k) ||
		        l < 0 && UIM(1U) << j < abs(l))
			    continue;
		    TEST("64U", (UIM(1U) << i) + k, (UIM(1U) << j) + l, ur64);
		}
	    }
	}
    }
    exit(0);
    return 0;
}
