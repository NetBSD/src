/*	$NetBSD: clock.h,v 1.8.18.1 2017/12/03 11:36:48 jdolecek Exp $ */
/*
 * Copyright (c) 1996 Ludd, University of Lule}, Sweden.
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
#ifndef _VAX_CLOCK_H_
#define	_VAX_CLOCK_H_

#include <dev/clock_subr.h>

/*
 * Time constants. These are unlikely to change.
 */
#define TODRBASE	(1 << 28) /* Rumours says it comes from VMS */

#define	SEC_OFF		0
#define	MIN_OFF		2
#define	HR_OFF		4
#define	WDAY_OFF	6
#define	DAY_OFF		7
#define	MON_OFF		8
#define	YR_OFF		9
#define	CSRA_OFF	10
#define	CSRB_OFF	11
#define	CSRD_OFF	13

#define	CSRA_UIP	0200
#define	CSRB_SET	0200
#define	CSRB_24		0002
#define	CSRB_DM		0004
#define	CSRD_VRT	0200

/* Var's used when dealing with clock chip */
extern	volatile short *clk_page;
extern	int clk_adrshift, clk_tweak;

/* Prototypes */
int generic_gettime(struct timeval *);
void generic_settime(struct timeval *);
int chip_gettime(struct timeval *);
void chip_settime(struct timeval *);
int yeartonum(int);
int numtoyear(int);

#endif /* _VAX_CLOCK_H_ */
