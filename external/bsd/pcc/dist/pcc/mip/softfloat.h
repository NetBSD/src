/*	Id: softfloat.h,v 1.2 2015/11/13 12:47:09 ragge Exp 	*/	
/*	$NetBSD: softfloat.h,v 1.1.1.1 2016/02/09 20:29:20 plunky Exp $	*/

/*
 * Copyright (c) 2015 Anders Magnusson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Definitions for softfloat routines.
 */


#define NFAARG	((SZLDOUBLE+SZINT+(SZINT-1))/SZINT)
#define	FP_TOP	(NFAARG-1)
union flt {
	long double fp;
	int fa[NFAARG];	/* one more than fp size */
};


union flt flt_zero;

#ifdef SOFTFLOAT
typedef struct softfloat SF;	
SF soft_neg(SF);
SF soft_cast(CONSZ v, TWORD);
SF soft_plus(SF, SF);
SF soft_minus(SF, SF);	
SF soft_mul(SF, SF);
SF soft_div(SF, SF);
int soft_cmp_eq(SF, SF);
int soft_cmp_ne(SF, SF);
int soft_cmp_ge(SF, SF);
int soft_cmp_gt(SF, SF);
int soft_cmp_le(SF, SF);
int soft_cmp_lt(SF, SF);
int soft_isz(SF);
CONSZ soft_val(SF);
#define FLOAT_NEG(sf)		soft_neg(sf)
#define FLOAT_CAST(v,t)		soft_cast(v, t)
#define FLOAT_PLUS(x1,x2)	soft_plus(x1, x2)
#define FLOAT_MINUS(x1,x2)	soft_minus(x1, x2)
#define FLOAT_MUL(x1,x2)	soft_mul(x1, x2)
#define FLOAT_DIV(x1,x2)	soft_div(x1, x2)
#define FLOAT_ISZERO(sf)	soft_isz(sf)
#define FLOAT_VAL(sf)		soft_val(sf)
#define FLOAT_EQ(x1,x2)		soft_cmp_eq(x1, x2)
#define FLOAT_NE(x1,x2)		soft_cmp_ne(x1, x2)
#define FLOAT_GE(x1,x2)		soft_cmp_ge(x1, x2)
#define FLOAT_GT(x1,x2)		soft_cmp_gt(x1, x2)
#define FLOAT_LE(x1,x2)		soft_cmp_le(x1, x2)
#define FLOAT_LT(x1,x2)		soft_cmp_lt(x1, x2)
#else	
#define FLOAT_NEG(p)		(p->fp = -p->fp)
#define FLOAT_INT2FP(d,p,v)	(ISUNSIGNED(v) ? \
	(d->fp = (long double)(U_CONSZ)(p)) : (d->fp = (long double)(CONSZ)(p)))
#define	FLOAT_FP2INT(i,d,t)	(ISUNSIGNED(t) ? \
	(i = (U_CONSZ)(d->fp)) : (i = d->fp))
#define	FLOAT_FP2FP(d,t)	(d->fp = (t == FLOAT ? (float)d->fp :	\
	t == DOUBLE ? (double)d->fp : d->fp))
#define FLOAT_PLUS(x,x1,x2)	(x->fp = (x1)->fp + (x2)->fp)
#define FLOAT_MINUS(x,x1,x2)	(x->fp = (x1)->fp - (x2)->fp)
#define FLOAT_MUL(x,x1,x2)	(x->fp = (x1)->fp * (x2)->fp)
#define FLOAT_DIV(x,x1,x2)	(x->fp = (x1)->fp / (x2)->fp)
#define FLOAT_ISZERO(p)		((p)->fp == 0.0)
#define FLOAT_VAL(p)		(CONSZ)(p)
#define FLOAT_EQ(x1,x2)		((x1)->fp == (x2)->fp)
#define FLOAT_NE(x1,x2)		((x1)->fp != (x2)->fp)
#define FLOAT_GE(x1,x2)		((x1)->fp >= (x2)->fp)
#define FLOAT_GT(x1,x2)		((x1)->fp > (x2)->fp)
#define FLOAT_LE(x1,x2)		((x1)->fp <= (x2)->fp)
#define FLOAT_LT(x1,x2)		((x1)->fp < (x2)->fp)
#define	FLOAT_ZERO		(&flt_zero)
#endif	

