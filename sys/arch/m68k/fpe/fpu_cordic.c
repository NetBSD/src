/*	$NetBSD: fpu_cordic.c,v 1.2.16.1 2016/10/05 20:55:31 skrll Exp $	*/

/*
 * Copyright (c) 2013 Tetsuya Isaki. All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fpu_cordic.c,v 1.2.16.1 2016/10/05 20:55:31 skrll Exp $");

#include <machine/ieee.h>

#include "fpu_emulate.h"

/*
 * sfpn = shoftened fp number; the idea is from fpu_log.c but not the same.
 * The most significant byte of sp_m0 is EXP (signed byte) and the rest
 * of sp_m0 is fp_mant[0].
 */
struct sfpn {
	uint32_t sp_m0;
	uint32_t sp_m1;
	uint32_t sp_m2;
};

#if defined(CORDIC_BOOTSTRAP)
/*
 * This is a bootstrap code to generate a pre-calculated tables such as
 * atan_table[] and atanh_table[].  However, it's just for reference.
 * If you want to run the bootstrap, you will define CORDIC_BOOTSTRAP
 * and modify these files as a userland application.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

static void prepare_cordic_const(struct fpemu *);
static struct fpn *fpu_gain1_cordic(struct fpemu *);
static struct fpn *fpu_gain2_cordic(struct fpemu *);
static struct fpn *fpu_atan_taylor(struct fpemu *);
static void printf_fpn(const struct fpn *);
static void printf_sfpn(const struct sfpn *);
static void fpn_to_sfpn(struct sfpn *, const struct fpn *);

static struct sfpn atan_table[EXT_FRACBITS];
static struct sfpn atanh_table[EXT_FRACBITS];
static struct fpn inv_gain1;
static struct fpn inv_gain2;

static void fpu_cordit2(struct fpemu *,
	struct fpn *, struct fpn *, struct fpn *, const struct fpn *);

int
main(int argc, char *argv[])
{
	struct fpemu dummyfe;
	int i;
	struct fpn fp;

	memset(&dummyfe, 0, sizeof(dummyfe));
	prepare_cordic_const(&dummyfe);

	/* output as source code */
	printf("static const struct sfpn atan_table[] = {\n");
	for (i = 0; i < EXT_FRACBITS; i++) {
		printf("\t");
		printf_sfpn(&atan_table[i]);
		printf(",\n");
	}
	printf("};\n\n");

	printf("static const struct sfpn atanh_table[] = {\n");
	for (i = 0; i < EXT_FRACBITS; i++) {
		printf("\t");
		printf_sfpn(&atanh_table[i]);
		printf(",\n");
	}
	printf("};\n\n");

	printf("const struct fpn fpu_cordic_inv_gain1 =\n\t");
	printf_fpn(&inv_gain1);
	printf(";\n\n");

	printf("const struct fpn fpu_cordic_inv_gain2 =\n\t");
	printf_fpn(&inv_gain2);
	printf(";\n\n");
}

/*
 * This routine uses fpu_const(), fpu_add(), fpu_div(), fpu_logn()
 * and fpu_atan_taylor() as bootstrap.
 */
static void
prepare_cordic_const(struct fpemu *fe)
{
	struct fpn t;
	struct fpn x;
	struct fpn *r;
	int i;

	/* atan_table and atanh_table */
	fpu_const(&t, FPU_CONST_1);
	for (i = 0; i < EXT_FRACBITS; i++) {
		/* atan(t) */
		CPYFPN(&fe->fe_f2, &t);
		r = fpu_atan_taylor(fe);
		fpn_to_sfpn(&atan_table[i], r);

		/* t /= 2 */
		t.fp_exp--;

		/* (1-t) */
		fpu_const(&fe->fe_f1, FPU_CONST_1);
		CPYFPN(&fe->fe_f2, &t);
		fe->fe_f2.fp_sign = 1;
		r = fpu_add(fe);
		CPYFPN(&x, r);

		/* (1+t) */
		fpu_const(&fe->fe_f1, FPU_CONST_1);
		CPYFPN(&fe->fe_f2, &t);
		r = fpu_add(fe);

		/* r = (1+t)/(1-t) */
		CPYFPN(&fe->fe_f1, r);
		CPYFPN(&fe->fe_f2, &x);
		r = fpu_div(fe);

		/* r = log(r) */
		CPYFPN(&fe->fe_f2, r);
		r = fpu_logn(fe);

		/* r /= 2 */
		r->fp_exp--;

		fpn_to_sfpn(&atanh_table[i], r);
	}

	/* inv_gain1 = 1 / gain1cordic() */
	r = fpu_gain1_cordic(fe);
	CPYFPN(&fe->fe_f2, r);
	fpu_const(&fe->fe_f1, FPU_CONST_1);
	r = fpu_div(fe);
	CPYFPN(&inv_gain1, r);

	/* inv_gain2 = 1 / gain2cordic() */
	r = fpu_gain2_cordic(fe);
	CPYFPN(&fe->fe_f2, r);
	fpu_const(&fe->fe_f1, FPU_CONST_1);
	r = fpu_div(fe);
	CPYFPN(&inv_gain2, r);
}

static struct fpn *
fpu_gain1_cordic(struct fpemu *fe)
{
	struct fpn x;
	struct fpn y;
	struct fpn z;
	struct fpn v;

	fpu_const(&x, FPU_CONST_1);
	fpu_const(&y, FPU_CONST_0);
	fpu_const(&z, FPU_CONST_0);
	CPYFPN(&v, &x);
	v.fp_sign = !v.fp_sign;

	fpu_cordit1(fe, &x, &y, &z, &v);
	CPYFPN(&fe->fe_f2, &x);
	return &fe->fe_f2;
}

static struct fpn *
fpu_gain2_cordic(struct fpemu *fe)
{
	struct fpn x;
	struct fpn y;
	struct fpn z;
	struct fpn v;

	fpu_const(&x, FPU_CONST_1);
	fpu_const(&y, FPU_CONST_0);
	fpu_const(&z, FPU_CONST_0);
	CPYFPN(&v, &x);
	v.fp_sign = !v.fp_sign;

	fpu_cordit2(fe, &x, &y, &z, &v);
	CPYFPN(&fe->fe_f2, &x);
	return &fe->fe_f2;
}

/*
 * arctan(x) = pi/4 (for |x| = 1)
 *
 *                 x^3   x^5   x^7
 * arctan(x) = x - --- + --- - --- + ...   (for |x| < 1)
 *                  3     5     7
 */
static struct fpn *
fpu_atan_taylor(struct fpemu *fe)
{
	struct fpn res;
	struct fpn x2;
	struct fpn s0;
	struct fpn *s1;
	struct fpn *r;
	uint32_t k;

	/* arctan(1) is pi/4 */
	if (fe->fe_f2.fp_exp == 0) {
		fpu_const(&fe->fe_f2, FPU_CONST_PI);
		fe->fe_f2.fp_exp -= 2;
		return &fe->fe_f2;
	}

	/* s0 := x */
	CPYFPN(&s0, &fe->fe_f2);

	/* res := x */
	CPYFPN(&res, &fe->fe_f2);

	/* x2 := x * x */
	CPYFPN(&fe->fe_f1, &fe->fe_f2);
	r = fpu_mul(fe);
	CPYFPN(&x2, r);

	k = 3;
	for (;;) {
		/* s1 := -s0 * x2 */
		CPYFPN(&fe->fe_f1, &s0);
		CPYFPN(&fe->fe_f2, &x2);
		s1 = fpu_mul(fe);
		s1->fp_sign ^= 1;
		CPYFPN(&fe->fe_f1, s1);

		/* s0 := s1 for next loop */
		CPYFPN(&s0, s1);

		/* s1 := s1 / k */
		fpu_explode(fe, &fe->fe_f2, FTYPE_LNG, &k);
		s1 = fpu_div(fe);

		/* break if s1 is enough small */
		if (ISZERO(s1))
			break;
		if (res.fp_exp - s1->fp_exp >= FP_NMANT)
			break;

		/* res += s1 */
		CPYFPN(&fe->fe_f2, s1);
		CPYFPN(&fe->fe_f1, &res);
		r = fpu_add(fe);
		CPYFPN(&res, r);

		k += 2;
	}

	CPYFPN(&fe->fe_f2, &res);
	return &fe->fe_f2;
}

static void
printf_fpn(const struct fpn *fp)
{
	printf("{ %d, %d, %3d, %d, { 0x%08x, 0x%08x, 0x%08x, }, }",
		fp->fp_class, fp->fp_sign, fp->fp_exp, fp->fp_sticky ? 1 : 0,
		fp->fp_mant[0], fp->fp_mant[1], fp->fp_mant[2]);
}

static void
printf_sfpn(const struct sfpn *sp)
{
	printf("{ 0x%08x, 0x%08x, 0x%08x, }",
		sp->sp_m0, sp->sp_m1, sp->sp_m2);
}

static void
fpn_to_sfpn(struct sfpn *sp, const struct fpn *fp)
{
	sp->sp_m0 = (fp->fp_exp << 24) | fp->fp_mant[0];
	sp->sp_m1 = fp->fp_mant[1];
	sp->sp_m2 = fp->fp_mant[2];
}

#else /* CORDIC_BOOTSTRAP */

static const struct sfpn atan_table[] = {
	{ 0xff06487e, 0xd5110b46, 0x11a80000, },
	{ 0xfe076b19, 0xc1586ed3, 0xda2b7f0d, },
	{ 0xfd07d6dd, 0x7e4b2037, 0x58ab6e33, },
	{ 0xfc07f56e, 0xa6ab0bdb, 0x719644b5, },
	{ 0xfb07fd56, 0xedcb3f7a, 0x71b65937, },
	{ 0xfa07ff55, 0x6eea5d89, 0x2a13bce7, },
	{ 0xf907ffd5, 0x56eedca6, 0xaddf3c5f, },
	{ 0xf807fff5, 0x556eeea5, 0xcb403117, },
	{ 0xf707fffd, 0x5556eeed, 0xca5d8956, },
	{ 0xf607ffff, 0x55556eee, 0xea5ca6ab, },
	{ 0xf507ffff, 0xd55556ee, 0xeedca5c8, },
	{ 0xf407ffff, 0xf555556e, 0xeeeea5c8, },
	{ 0xf307ffff, 0xfd555556, 0xeeeeedc8, },
	{ 0xf207ffff, 0xff555555, 0x6eeeeee8, },
	{ 0xf107ffff, 0xffd55555, 0x56eeeeed, },
	{ 0xf007ffff, 0xfff55555, 0x556eeeed, },
	{ 0xef07ffff, 0xfffd5555, 0x5556eeed, },
	{ 0xee07ffff, 0xffff5555, 0x55556eed, },
	{ 0xed07ffff, 0xffffd555, 0x555556ed, },
	{ 0xec07ffff, 0xfffff555, 0x5555556d, },
	{ 0xeb07ffff, 0xfffffd55, 0x55555555, },
	{ 0xea07ffff, 0xffffff55, 0x55555554, },
	{ 0xe907ffff, 0xffffffd5, 0x55555554, },
	{ 0xe807ffff, 0xfffffff5, 0x55555554, },
	{ 0xe707ffff, 0xfffffffd, 0x55555554, },
	{ 0xe607ffff, 0xffffffff, 0x55555554, },
	{ 0xe507ffff, 0xffffffff, 0xd5555554, },
	{ 0xe407ffff, 0xffffffff, 0xf5555554, },
	{ 0xe307ffff, 0xffffffff, 0xfd555554, },
	{ 0xe207ffff, 0xffffffff, 0xff555554, },
	{ 0xe107ffff, 0xffffffff, 0xffd55554, },
	{ 0xe007ffff, 0xffffffff, 0xfff55554, },
	{ 0xdf07ffff, 0xffffffff, 0xfffd5554, },
	{ 0xde07ffff, 0xffffffff, 0xffff5554, },
	{ 0xdd07ffff, 0xffffffff, 0xffffd554, },
	{ 0xdc07ffff, 0xffffffff, 0xfffff554, },
	{ 0xdb07ffff, 0xffffffff, 0xfffffd54, },
	{ 0xda07ffff, 0xffffffff, 0xffffff54, },
	{ 0xd907ffff, 0xffffffff, 0xffffffd4, },
	{ 0xd807ffff, 0xffffffff, 0xfffffff4, },
	{ 0xd707ffff, 0xffffffff, 0xfffffffc, },
	{ 0xd7040000, 0x00000000, 0x00000000, },
	{ 0xd6040000, 0x00000000, 0x00000000, },
	{ 0xd5040000, 0x00000000, 0x00000000, },
	{ 0xd4040000, 0x00000000, 0x00000000, },
	{ 0xd3040000, 0x00000000, 0x00000000, },
	{ 0xd2040000, 0x00000000, 0x00000000, },
	{ 0xd1040000, 0x00000000, 0x00000000, },
	{ 0xd0040000, 0x00000000, 0x00000000, },
	{ 0xcf040000, 0x00000000, 0x00000000, },
	{ 0xce040000, 0x00000000, 0x00000000, },
	{ 0xcd040000, 0x00000000, 0x00000000, },
	{ 0xcc040000, 0x00000000, 0x00000000, },
	{ 0xcb040000, 0x00000000, 0x00000000, },
	{ 0xca040000, 0x00000000, 0x00000000, },
	{ 0xc9040000, 0x00000000, 0x00000000, },
	{ 0xc8040000, 0x00000000, 0x00000000, },
	{ 0xc7040000, 0x00000000, 0x00000000, },
	{ 0xc6040000, 0x00000000, 0x00000000, },
	{ 0xc5040000, 0x00000000, 0x00000000, },
	{ 0xc4040000, 0x00000000, 0x00000000, },
	{ 0xc3040000, 0x00000000, 0x00000000, },
	{ 0xc2040000, 0x00000000, 0x00000000, },
	{ 0xc1040000, 0x00000000, 0x00000000, },
};

static const struct sfpn atanh_table[] = {
	{ 0xff0464fa, 0x9eab40c2, 0xa5dc43f6, },
	{ 0xfe04162b, 0xbea04514, 0x69ca8e4a, },
	{ 0xfd040562, 0x4727abbd, 0xda654b67, },
	{ 0xfc040156, 0x22b4dd6b, 0x372a679c, },
	{ 0xfb040055, 0x62246bb8, 0x92d60b35, },
	{ 0xfa040015, 0x56222b47, 0x2637d656, },
	{ 0xf9040005, 0x55622246, 0xb4dcf86e, },
	{ 0xf8040001, 0x55562222, 0xb46bb307, },
	{ 0xf7040000, 0x55556222, 0x246b45cd, },
	{ 0xf6040000, 0x15555622, 0x222b465b, },
	{ 0xf5040000, 0x05555562, 0x2222467f, },
	{ 0xf4040000, 0x01555556, 0x22221eaf, },
	{ 0xf3040000, 0x00555555, 0x62222213, },
	{ 0xf2040000, 0x00155555, 0x56221221, },
	{ 0xf1040000, 0x00055555, 0x556221a2, },
	{ 0xf0040000, 0x00015555, 0x5556221e, },
	{ 0xef040000, 0x00005555, 0x55552222, },
	{ 0xee040000, 0x00001555, 0x55555222, },
	{ 0xed040000, 0x00000555, 0x55555522, },
	{ 0xec040000, 0x00000155, 0x55555552, },
	{ 0xeb040000, 0x00000055, 0x554d5555, },
	{ 0xea040000, 0x00000015, 0x55545555, },
	{ 0xe9040000, 0x00000005, 0x55553555, },
	{ 0xe8040000, 0x00000001, 0x55555155, },
	{ 0xe7040000, 0x00000000, 0x555554d5, },
	{ 0xe6040000, 0x00000000, 0x15555545, },
	{ 0xe5040000, 0x00000000, 0x05555553, },
	{ 0xe307ffff, 0xffffffff, 0xfaaaaaaa, },
	{ 0xe207ffff, 0xffffffff, 0xfeaaaaaa, },
	{ 0xe107ffff, 0xffffffff, 0xffaaaaaa, },
	{ 0xe007ffff, 0xffffffff, 0xffeaaaaa, },
	{ 0xdf07ffff, 0xffffffff, 0xfffaaaaa, },
	{ 0xde07ffff, 0xffffffff, 0xfffeaaaa, },
	{ 0xdd07ffff, 0xffffffff, 0xffffaaaa, },
	{ 0xdc07ffff, 0xffffffff, 0xffffeaaa, },
	{ 0xdb07ffff, 0xffffffff, 0xfffffaaa, },
	{ 0xda07ffff, 0xffffffff, 0xfffffeaa, },
	{ 0xd907ffff, 0xffffffff, 0xffffffaa, },
	{ 0xd807ffff, 0xffffffff, 0xffffffea, },
	{ 0xd707ffff, 0xffffffff, 0xfffffffa, },
	{ 0xd607ffff, 0xffffffff, 0xfffffffe, },
	{ 0xd507ffff, 0xfffffe00, 0x00000000, },
	{ 0xd407ffff, 0xffffff00, 0x00000000, },
	{ 0xd307ffff, 0xffffff80, 0x00000000, },
	{ 0xd207ffff, 0xffffffc0, 0x00000000, },
	{ 0xd107ffff, 0xffffffe0, 0x00000000, },
	{ 0xd007ffff, 0xfffffff0, 0x00000000, },
	{ 0xcf07ffff, 0xfffffff8, 0x00000000, },
	{ 0xce07ffff, 0xfffffffc, 0x00000000, },
	{ 0xcd07ffff, 0xfffffffe, 0x00000000, },
	{ 0xcc07ffff, 0xffffffff, 0x00000000, },
	{ 0xcb07ffff, 0xffffffff, 0x80000000, },
	{ 0xca07ffff, 0xffffffff, 0xc0000000, },
	{ 0xc907ffff, 0xffffffff, 0xe0000000, },
	{ 0xc807ffff, 0xffffffff, 0xf0000000, },
	{ 0xc707ffff, 0xffffffff, 0xf8000000, },
	{ 0xc607ffff, 0xffffffff, 0xfc000000, },
	{ 0xc507ffff, 0xffffffff, 0xfe000000, },
	{ 0xc407ffff, 0xffffffff, 0xff000000, },
	{ 0xc307ffff, 0xffffffff, 0xff800000, },
	{ 0xc207ffff, 0xffffffff, 0xffc00000, },
	{ 0xc107ffff, 0xffffffff, 0xffe00000, },
	{ 0xc007ffff, 0xffffffff, 0xfff00000, },
	{ 0xbf07ffff, 0xffffffff, 0xfff80000, },
};

const struct fpn fpu_cordic_inv_gain1 =
	{ 1, 0,  -1, 1, { 0x0004dba7, 0x6d421af2, 0xd33fafd1, }, };

const struct fpn fpu_cordic_inv_gain2 =
	{ 1, 0,   0, 1, { 0x0004d483, 0xec3803fc, 0xc5ff12f8, }, };

#endif /* CORDIC_BOOTSTRAP */

static inline void
sfpn_to_fpn(struct fpn *fp, const struct sfpn *s)
{
	fp->fp_class = FPC_NUM;
	fp->fp_sign = 0;
	fp->fp_sticky = 0;
	fp->fp_exp = s->sp_m0 >> 24;
	if (fp->fp_exp & 0x80) {
		fp->fp_exp |= 0xffffff00;
	}
	fp->fp_mant[0] = s->sp_m0 & 0x000fffff;
	fp->fp_mant[1] = s->sp_m1;
	fp->fp_mant[2] = s->sp_m2;
}

void
fpu_cordit1(struct fpemu *fe, struct fpn *x0, struct fpn *y0, struct fpn *z0,
	const struct fpn *vecmode)
{
	struct fpn t;
	struct fpn x;
	struct fpn y;
	struct fpn z;
	struct fpn *r;
	int i;
	int sign;

	fpu_const(&t, FPU_CONST_1);
	CPYFPN(&x, x0);
	CPYFPN(&y, y0);
	CPYFPN(&z, z0);

	for (i = 0; i < EXT_FRACBITS; i++) {
		struct fpn x1;

		/* y < vecmode */
		CPYFPN(&fe->fe_f1, &y);
		CPYFPN(&fe->fe_f2, vecmode);
		fe->fe_f2.fp_sign = !fe->fe_f2.fp_sign;
		r = fpu_add(fe);

		if ((vecmode->fp_sign == 0 && r->fp_sign) ||
		    (vecmode->fp_sign && z.fp_sign == 0)) {
			sign = 1;
		} else {
			sign = 0;
		}

		/* y * t */
		CPYFPN(&fe->fe_f1, &y);
		CPYFPN(&fe->fe_f2, &t);
		r = fpu_mul(fe);

		/*
		 * x1 = x - y*t (if sign)
		 * x1 = x + y*t
		 */
		CPYFPN(&fe->fe_f2, r);
		if (sign)
			fe->fe_f2.fp_sign = !fe->fe_f2.fp_sign;
		CPYFPN(&fe->fe_f1, &x);
		r = fpu_add(fe);
		CPYFPN(&x1, r);

		/* x * t */
		CPYFPN(&fe->fe_f1, &x);
		CPYFPN(&fe->fe_f2, &t);
		r = fpu_mul(fe);

		/*
		 * y = y + x*t (if sign)
		 * y = y - x*t
		 */
		CPYFPN(&fe->fe_f2, r);
		if (!sign)
			fe->fe_f2.fp_sign = !fe->fe_f2.fp_sign;
		CPYFPN(&fe->fe_f1, &y);
		r = fpu_add(fe);
		CPYFPN(&y, r);

		/*
		 * z = z - atan_table[i] (if sign)
		 * z = z + atan_table[i]
		 */
		CPYFPN(&fe->fe_f1, &z);
		sfpn_to_fpn(&fe->fe_f2, &atan_table[i]);
		if (sign)
			fe->fe_f2.fp_sign = !fe->fe_f2.fp_sign;
		r = fpu_add(fe);
		CPYFPN(&z, r);

		/* x = x1 */
		CPYFPN(&x, &x1);

		/* t /= 2 */
		t.fp_exp--;
	}

	CPYFPN(x0, &x);
	CPYFPN(y0, &y);
	CPYFPN(z0, &z);
}

#if defined(CORDIC_BOOTSTRAP)
static void
fpu_cordit2(struct fpemu *fe, struct fpn *x0, struct fpn *y0, struct fpn *z0,
	const struct fpn *vecmode)
{
	struct fpn t;
	struct fpn x;
	struct fpn y;
	struct fpn z;
	struct fpn *r;
	int i;
	int k;
	int sign;

	/* t = 0.5 */
	fpu_const(&t, FPU_CONST_1);
	t.fp_exp--;

	CPYFPN(&x, x0);
	CPYFPN(&y, y0);
	CPYFPN(&z, z0);

	k = 3;
	for (i = 0; i < EXT_FRACBITS; i++) {
		struct fpn x1;
		int j;

		for (j = 0; j < 2; j++) {
			if ((vecmode->fp_sign == 0 && y.fp_sign) ||
			    (vecmode->fp_sign && z.fp_sign == 0)) {
				sign = 0;
			} else {
				sign = 1;
			}

			/* y * t */
			CPYFPN(&fe->fe_f1, &y);
			CPYFPN(&fe->fe_f2, &t);
			r = fpu_mul(fe);

			/*
			 * x1 = x + y*t
			 * x1 = x - y*t (if sign)
			 */
			CPYFPN(&fe->fe_f2, r);
			if (sign)
				fe->fe_f2.fp_sign = !fe->fe_f2.fp_sign;
			CPYFPN(&fe->fe_f1, &x);
			r = fpu_add(fe);
			CPYFPN(&x1, r);

			/* x * t */
			CPYFPN(&fe->fe_f1, &x);
			CPYFPN(&fe->fe_f2, &t);
			r = fpu_mul(fe);

			/*
			 * y = y + x*t
			 * y = y - x*t (if sign)
			 */
			CPYFPN(&fe->fe_f2, r);
			if (sign)
				fe->fe_f2.fp_sign = !fe->fe_f2.fp_sign;
			CPYFPN(&fe->fe_f1, &y);
			r = fpu_add(fe);
			CPYFPN(&y, r);

			/*
			 * z = z + atanh_table[i] (if sign)
			 * z = z - atanh_table[i]
			 */
			CPYFPN(&fe->fe_f1, &z);
			sfpn_to_fpn(&fe->fe_f2, &atanh_table[i]);
			if (!sign)
				fe->fe_f2.fp_sign = !fe->fe_f2.fp_sign;
			r = fpu_add(fe);
			CPYFPN(&z, r);

			/* x = x1 */
			CPYFPN(&x, &x1);

			if (k > 0) {
				k--;
				break;
			} else {
				k = 3;
			}
		}

		/* t /= 2 */
		t.fp_exp--;
	}

	CPYFPN(x0, &x);
	CPYFPN(y0, &y);
	CPYFPN(z0, &z);
}
#endif /* CORDIC_BOOTSTRAP */
