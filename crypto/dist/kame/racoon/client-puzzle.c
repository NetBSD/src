#include <sys/types.h>
#include <openssl/sha.h>

#include <stdio.h>
#include <stdlib.h>

#include "vmbuf.h"

vchar_t *mdx __P((const vchar_t *, int));
void plusone __P((u_char *, int));
int islzero __P((char *, int));

#ifdef DEBUG
#include <sys/time.h>
double stats;
double timedelta __P((struct timeval *, struct timeval *));
double
timedelta(t1, t2)
        struct timeval *t1, *t2;
{
	if (t2->tv_usec >= t1->tv_usec)
		return t2->tv_sec - t1->tv_sec +
			(double)(t2->tv_usec - t1->tv_usec) / 1000000;

	return t2->tv_sec - t1->tv_sec - 1 +
		(double)(1000000 + t2->tv_usec - t1->tv_usec) / 1000000;
}
#endif

#include <time.h>
int
main(ac, av)
	int ac;
	char **av;
{
	int k = 0, n = 1;
	int datalen = 16;	/*XXX*/
	vchar_t *data, *res;
	int i, j;

	switch (ac) {
	default:
	case 3:
		n = atoi(*(av + 2));
	case 2:
		k = atoi(*(av + 1));
		break;
	case 1:
		printf("Usage: client-puzzle (size) (times)\n");
		printf("\tsize : the length of MSB to be zero.\n");
		printf("\ttimes: the number of times of testing.\n");
		exit(0);
	}

	data = vmalloc(16);	/*XXX*/
	if (data == NULL)
		return -1;

	srandom(time(NULL));
	for (i = 0; i < n; i++) {
		for (j = 0; j < datalen; j++)
			data->v[j] = (char)random();

		res = mdx((const vchar_t *)data, k);
		if (res == NULL)
			return -1;
	}

	return 0;
}

vchar_t *
mdx(data, k)
	const vchar_t *data;
	int k;
{
	SHA_CTX c;
	vchar_t *sub, *res;
	u_long n, max = ~0;
	int last;

	sub = vmalloc(SHA_DIGEST_LENGTH);
	if (sub == NULL)
		return NULL;

	/*XXX how many length should be allocated ?*/
	res = vmalloc(SHA_DIGEST_LENGTH);
	if (res == NULL)
		return NULL;
	memset(res->v, 0, res->l);

	last = res->l - 1;
	for (n = 0; n < max; n ++) {

		if (n & 1)
			res->v[last] |= 1;

#ifdef DEBUG
    {
		struct timeval start, end;
		gettimeofday(&start, NULL);
#endif
		SHA1_Init(&c);
		SHA1_Update((SHA_CTX *)&c, data->v, data->l);
		SHA1_Update((SHA_CTX *)&c, res->v, res->l);
		SHA1_Final(sub->v, (SHA_CTX *)&c);
#ifdef DEBUG
		gettimeofday(&end, NULL);
		stats += timedelta(&start, &end);
    }
#endif

		if (islzero(sub->v, k))
			goto found;
		if (n & 1) {
#ifdef DEBUG2
			if (n > 0xfffff0) {
				int j;
				for (j = 0; j < res->l; j++)
					printf("%02x", (u_char)res->v[j]);
				printf("\n");
			}
#endif
			plusone(res->v, res->l);
#ifdef DEBUG2
			if (n > 0xfffff0) {
				int j;
				for (j = 0; j < res->l; j++)
					printf("%02x", (u_char)res->v[j]);
				printf("\n");
			}
#endif
		}
	}

    found:
#ifdef DEBUG
	if (n != max)
	{
#ifdef DEBUG2
		int i;
		printf("dat=");
		for (i = 0; i < data->l; i++)
			printf("%02x", (u_char)(data->v[i] & 0xff));
		printf("\n");
		printf("sub=");
		for (i = 0; i < sub->l; i++)
			printf("%02x", (u_char)(sub->v[i] & 0xff));
		printf("\n");
		printf("res=");
		for (i = 0; i < res->l; i++)
			printf("%02x", (u_char)(res->v[i] & 0xff));
		printf("\n");
#endif
		printf("k=%d\tn=%ld\ttotal=%9.6f(s)\tavg=%9.6f(s)\n",
			k, n + 1, stats, stats/(n + 1));
	}
#endif
	vfree(sub);

	return n == max ? NULL : res;
}

/*
 * d: pointer of the data.
 * l: the length of bytes.
 */
void
plusone(d, l)
	u_char *d;
	int l;
{
	int carry = 0;
	int i;

	if (l == 0)
		return;

	if (d[l - 1] == 0xff)
		carry = 1;
	d[l - 1]++;

	if (carry && l > 1) {
		carry = 0;
		for (i = l - 2; i >= 0; i--) {
			if (d[i] == 0xff)
				carry = 1;
			d[i]++;
			if (!carry)
				break;
			carry = 0;
		}
	}
}

/*
 * d: pointer of the data.
 * k: the length of most significant bits to be zero.
 * return value:
 * 1: match.
 * 0: not match.
 */
int
islzero(d, k)
	char *d;
	int k;
{
	while (k >= 8) {
		if (*d++ != 0)
			return 0;
		k -= 8;
	}

	if (k > 0) {
		if (*d != (0xff >> k))
			return 0;
	}

	return 1;
}
