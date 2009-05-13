/* $NetBSD: rbstress.c,v 1.1.2.2 2009/05/13 19:18:44 jym Exp $ */

#include <sys/cdefs.h>
#include <sys/tree.h>
#include <stdlib.h>
#include <stdio.h>

struct mist {
	RB_ENTRY(mist) rbentry;
	int key;
};
RB_HEAD(head, mist) tt;

static int
mistcmp(struct mist *a, struct mist *b)
{
#if 0
	return (b->key - a->key); /* wrong, can overflow */
#else
	if (b->key > a->key)
		return 1;
	else if (b->key < a->key)
		return (-1);
	else
		return 0;
#endif
}

RB_PROTOTYPE(head, mist, rbentry, mistcmp)
RB_GENERATE(head, mist, rbentry, mistcmp)

static struct mist *
addmist(int key)
{
	struct mist *m;

	m = malloc(sizeof(struct mist));
	m->key = key;
	RB_INSERT(head, &tt, m);
	return m;
}

static int
findmist(struct mist *m)
{

	return (!!RB_FIND(head, &tt, m));
}

#define N 1000
static int
test(void)
{
	struct mist *m[N];
	int fail, i, j;

	RB_INIT(&tt);
	fail = 0;
	for (i = 0; i < N; i++) {
		m[i] = addmist(random() << 1); /* use all 32 bits */
		for (j = 0; j <= i; j++)
			if (!findmist(m[j]))
				fail++;
	}
	return fail;
}

int
main(int argc, char **argv)
{
	int i, fail, f;

	srandom(4711);
	fail = 0;
	for (i = 0; i < 10; i++) {
		f = test();
		if (f) {
			printf("loop %d: %d errors\n", i, f);
			fail += f;
		}
	}
	exit(!!fail);
}
