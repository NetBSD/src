/* $Id: memset.c,v 1.1.2.1 1998/10/15 02:10:24 nisimura Exp $ */

#include <sys/types.h>

void *memset __P((void *, int, size_t));

#define WORD(x, y)	*(u_int32_t *)((x) + (y))
#define	FRACTION(x)	((0 - (int)(x)) & 3)
#define	CHUNKCNT(x, y)	((x) & -(y))

void *
memset(a0, a1, a2)
	void *a0;
	int a1;
	size_t a2;
{
	void *v0;
	unsigned t0, v1;

	v0 = a0;
	if (a2 >= 12) {
		if ((v1 = (u_int8_t)a1) != 0) {
			v1 |= (v1 << 8);
			v1 |= (v1 << 16);
		}
		t0 = FRACTION(a0);
		if (t0 > 0) {
			a2 -= t0;
			asm("swr %0,0(%1)" :: "r"(v1), "r"(a0));
			a0 = a0 + t0;
		}
		t0 = CHUNKCNT(a2, 128);
		if (t0 > 0) {
			a2 -= t0;
			t0 = (unsigned)a0 + t0;
			do {
				WORD(a0, 0) = v1;
				WORD(a0, 4) = v1;
				WORD(a0, 8) = v1;
				WORD(a0, 12) = v1;
				WORD(a0, 16) = v1;
				WORD(a0, 20) = v1;
				WORD(a0, 24) = v1;
				WORD(a0, 28) = v1;
				WORD(a0, 32) = v1;
				WORD(a0, 36) = v1;
				WORD(a0, 40) = v1;
				WORD(a0, 44) = v1;
				WORD(a0, 48) = v1;
				WORD(a0, 52) = v1;
				WORD(a0, 56) = v1;
				WORD(a0, 60) = v1;
				WORD(a0, 64) = v1;
				WORD(a0, 68) = v1;
				WORD(a0, 72) = v1;
				WORD(a0, 76) = v1;
				WORD(a0, 80) = v1;
				WORD(a0, 84) = v1;
				WORD(a0, 88) = v1;
				WORD(a0, 92) = v1;
				WORD(a0, 96) = v1;
				WORD(a0, 100) = v1;
				WORD(a0, 104) = v1;
				WORD(a0, 108) = v1;
				WORD(a0, 112) = v1;
				WORD(a0, 116) = v1;
				WORD(a0, 120) = v1;
				WORD(a0, 124) = v1;
				a0 = a0 + 128;
			} while (a0 != (void *)t0);
		}
		t0 = CHUNKCNT(a2, 32);
		if (t0 > 0) {
			a2 -= t0;
			t0 = (unsigned)a0 + t0;
			do {
				WORD(a0, 0) = v1;
				WORD(a0, 4) = v1;
				WORD(a0, 8) = v1;
				WORD(a0, 12) = v1;
				WORD(a0, 16) = v1;
				WORD(a0, 20) = v1;
				WORD(a0, 24) = v1;
				WORD(a0, 28) = v1;
				a0 = a0 + 32;
			} while (a0 != (void *)t0);
		}
		t0 = CHUNKCNT(a2, 4);
		if (t0 > 0) {
			a2 -= t0;
			t0 = (unsigned)a0 + t0;
			do {
				WORD(a0, 0) = v1;
				a0 = a0 + 4;
			} while (a0 != (void *)t0);
		}
	}
	t0 = (unsigned)a0 + a2;
	do {
		*(u_int8_t *)a0 = a1;
		a0 = a0 + 1;
	} while (a0 != (void *)t0);
	return v0;
}
