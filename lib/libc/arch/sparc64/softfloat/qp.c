/* $NetBSD: qp.c,v 1.1.2.2 2002/03/22 20:41:55 nathanw Exp $ */

#include <sys/cdefs.h>
#include <memory.h>

#include "milieu.h"
#include "softfloat.h"


void _Qp_add(float128 *c, float128 *a, float128 *b);

void _Qp_add(float128 *c, float128 *a, float128 *b)
{
	 *c =  float128_add(*a, *b);
}


int _Qp_cmp(float128 *a, float128 *b);

int _Qp_cmp(float128 *a, float128 *b)
{

	if (float128_eq(*a, *b))
		return 0;

	if (float128_le(*a, *b))
		return 1;

	return 2;
}


/*
 * XXX 
 */
int _Qp_cmpe(float128 *a, float128 *b);

int _Qp_cmpe(float128 *a, float128 *b)
{
	return _Qp_cmp(a, b);
}


void _Qp_div(float128 *c, float128 *a, float128 *b);

void _Qp_div(float128 *c, float128 *a, float128 *b)
{
	*c = float128_div(*a, *b);
}


void _Qp_dtoq(float128 *c, float64 *a);

void _Qp_dtoq(float128 *c, float64 *a)
{
	*c = float64_to_float128(*a);
}



int _Qp_feq(float128 *a, float128 *b);

int _Qp_feq(float128 *a, float128 *b)
{
	return float128_eq(*a, *b);
}


int _Qp_fge(float128 *a, float128 *b);

int _Qp_fge(float128 *a, float128 *b)
{
	return float128_le(*b, *a);
}


int _Qp_fgt(float128 *a, float128 *b);

int _Qp_fgt(float128 *a, float128 *b)
{
	return float128_lt(*b, *a);
}


int _Qp_fle(float128 *a, float128 *b);

int _Qp_fle(float128 *a, float128 *b)
{
	return float128_le(*a, *b);
}


int _Qp_flt(float128 *a, float128 *b);

int _Qp_flt(float128 *a, float128 *b)
{
	return float128_lt(*a, *b);
}


int _Qp_fne(float128 *a, float128 *b);

int _Qp_fne(float128 *a, float128 *b)
{
	return !float128_eq(*a, *b);
}


void _Qp_itoq(float128 *c, int a);

void _Qp_itoq(float128 *c, int a)
{
	*c = int32_to_float128(a);
}



void _Qp_mul(float128 *c, float128 *a, float128 *b);

void _Qp_mul(float128 *c, float128 *a, float128 *b)
{
	*c = float128_mul(*a, *b);
}


/*
 * XXX easy way to do this, softfloat function
 */
void _Qp_neg(float128 *c, float128 *a);

static float128 __zero = {0x4034000000000000, 0x00000000};

void _Qp_neg(float128 *c, float128 *a)
{
	*c = float128_sub(__zero, *a);
}



double _Qp_qtod(float128 *a);

double _Qp_qtod(float128 *a)
{
	float64 _c;
	double c;

	_c = float128_to_float64(*a);

	memcpy(&c, &_c, sizeof(double));

	return c;
}


int _Qp_qtoi(float128 *a);

int _Qp_qtoi(float128 *a)
{
	return float128_to_int32(*a);
}


float _Qp_qtos(float128 *a);

float _Qp_qtos(float128 *a)
{
	float c;
	float32 _c;

	_c = float128_to_float32(*a);

	memcpy(&c, &_c, sizeof(_c));

	return c; 
}


unsigned int _Qp_qtoui(float128 *a);

unsigned int _Qp_qtoui(float128 *a)
{
	return (unsigned int)float128_to_int32(*a);
}



unsigned long _Qp_qtoux(float128 *a);

unsigned long _Qp_qtoux(float128 *a)
{
	return (unsigned long)float128_to_int64(*a);
}



long _Qp_qtox(float128 *a);

long _Qp_qtox(float128 *a)
{
	return (long)float128_to_int64(*a);
}


void _Qp_sqrt(float128 *c, float128 *a);

void _Qp_sqrt(float128 *c, float128 *a)
{
	*c = float128_sqrt(*a);
}


void _Qp_stoq(float128 *c, float a);

void _Qp_stoq(float128 *c, float a)
{
	float32 _a;

	memcpy(&_a, &a, sizeof(a));

	*c = float32_to_float128(_a);
}


void _Qp_sub(float128 *c, float128 *a, float128 *b);

void _Qp_sub(float128 *c, float128 *a, float128 *b)
{
	*c = float128_sub(*a, *b);
}


void _Qp_uitoq(float128 *c, unsigned int a);

void _Qp_uitoq(float128 *c, unsigned int a)
{
	*c = int32_to_float128(a);
}


void _Qp_uxtoq(float128 *c, unsigned long a);

void _Qp_uxtoq(float128 *c, unsigned long a)
{
	*c = int64_to_float128(a);
}


void _Qp_xtoq(float128 *c, long a);

void _Qp_xtoq(float128 *c, long a)
{
	*c = int64_to_float128(a);
}
