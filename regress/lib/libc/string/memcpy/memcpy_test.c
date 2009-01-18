#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <md5.h>

#define	ALIGNMENTS 16
#define	LENGTHS	    4
#define BLOCKTYPES 4

MD5_CTX mc[1];

typedef	unsigned char testBlock_t[ALIGNMENTS * LENGTHS];

testBlock_t bss1, bss2;

unsigned char *start[BLOCKTYPES] = {
		bss1, bss2
};

char result[100];
const char goodResult[] = "7b405d24bc03195474c70ddae9e1f8fb";

void runTest(unsigned char *, unsigned char *);

int
main(int ac, char **av)
{
	int i, j;
	testBlock_t auto1, auto2;

	start[2] = auto1;
	start[3] = auto2;

	srandom(0L);
	MD5Init(mc);
	for (i = 0; i < BLOCKTYPES; ++i)
		for (j = 0; j < BLOCKTYPES; ++j)
			if (i != j)
				runTest(start[i], start[j]);
	MD5End(mc, result);
	return strcmp(result, goodResult);
}

void runTest(unsigned char *b1, unsigned char *b2)
{
	int	i, j, k, m;
	size_t	n;

	for (i = 0; i < ALIGNMENTS; ++i) {
		for (j = 0; j < ALIGNMENTS; ++j) {
			k = sizeof(testBlock_t) - (i > j ? i : j);
			for (m = 0; m < k; ++m) {
				for (n = 0; n < sizeof(testBlock_t); ++n) {
					b1[n] = (unsigned char)random();
					b2[n] = (unsigned char)random();
				}
				memcpy(b1 + i, b2 + j, m);
				MD5Update(mc, b1, sizeof(testBlock_t));
				MD5Update(mc, b2, sizeof(testBlock_t));
			}
		}
	}
}
