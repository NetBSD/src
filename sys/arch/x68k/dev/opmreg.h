/*	$NetBSD: opmreg.h,v 1.3 2001/05/02 13:00:20 minoura Exp $	*/

/*
 * Copyright (c) 1995 Masanobu Saitoh, Takuya Harakawa.
 * All rights reserved.
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
 *	This product includes software developed by Masanobu Saitoh.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _OPMREG_H_
#define _OPMREG_H_
/*
 * OPM voice structure
 */
struct opm_operator {
	u_char ar;
	u_char d1r;
	u_char d2r;
	u_char rr;
	u_char d1l;
	u_char tl;
	u_char ks;
	u_char mul;
	u_char dt1;
	u_char dt2;
	u_char ame;
};

struct opm_voice {
	struct opm_operator m1;
	struct opm_operator c1;
	struct opm_operator m2;
	struct opm_operator c2;
	u_char con;	/* connection */
	u_char fb;	/* feedback level */
	u_char sm;	/* slot mask */
};

/* XXX */

#define	OPM1B_CT1MSK	(0x80)
#define	OPM1B_CT2MSK	(0x40)

#define VS_CLK_8MHZ	(0x00)
#define VS_CLK_4MHZ	(0x80)

#define	FDCSTBY	(0x00)
#define	FDCRDY	(0x40)

void adpcm_chgclk	__P((u_char));
void fdc_force_ready	__P((u_char));

#define OPM_REG		0
#define OPM_DATA	1

#endif /* !_OPMREG_H_ */
