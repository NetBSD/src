/*	$NetBSD: vaxfp.h,v 1.4 2004/03/18 13:59:14 kleink Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * vaxfp.h defines the layout of VAX Floating-Point data types.
 * Only F_floating and D_floating types are defined here;
 * G_floating and H_floating are not supported by NetBSD.
 */
#ifndef _VAX_VAXFP_H_
#define	_VAX_VAXFP_H_

#define	FFLT_EXPBITS		8
#define	FFLT_FRACBITS		23

#define	DFLT_EXPBITS		8
#define	DFLT_FRACBITS		55

struct vax_f_floating {
	unsigned int	fflt_frach:7;
	unsigned int	fflt_exp:8;
	unsigned int	fflt_sign:1;
	unsigned int	fflt_fracl:16;
};

struct vax_d_floating {
	unsigned int	dflt_frach:7;
	unsigned int	dflt_exp:8;
	unsigned int	dflt_sign:1;
	unsigned int	dflt_fracm:16;
	unsigned int	dflt_fracl;
};

/*
 * Exponent biases.
 */
#define	FFLT_EXP_BIAS	128
#define	DFLT_EXP_BIAS	128

/*
 * Convenience data structures.
 */
union vax_ffloating_u {
	float			ffltu_f;
	struct vax_f_floating	ffltu_fflt;
};

union vax_dfloating_u {
	double			dfltu_d;
	struct vax_d_floating	dfltu_dflt;
};

#endif /* _VAX_VAXFP_H_ */
