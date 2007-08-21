/* $NetBSD: round.c,v 1.1 2007/08/21 19:52:36 drochner Exp $ */

#include <math.h>

/*
 * This tests for a bug in the initial implementation where precision
 * was lost in an internal substraction, leading to rounding
 * into the wrong direction.
 */

/* 0.5 - EPSILON */
#define VAL 0x0.7ffffffffffffcp0
#define VALF 0x0.7fffff8p0

int main()
{
	double a = VAL, b, c;
	float af = VALF, bf, cf;

	b = round(a);
	bf = roundf(af);
	c = round(-a);
	cf = roundf(-af);
	return (b != 0 || bf != 0 || c != 0 || cf != 0);
}
