/*	$NetBSD: bootinfo.h,v 1.1.2.2 2002/02/11 20:07:49 jdolecek Exp $	*/

/*-
 * Copyright (c) 1999-2001
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 */

#ifndef _HPC_BOOTINFO_H_
#define _HPC_BOOTINFO_H_

struct bootinfo {
	short length;
	short reserved;
	int magic;
	void *fb_addr;
	short fb_line_bytes;
	short fb_width;
	short fb_height;
	short fb_type;

	short bi_cnuse;
	unsigned long platid_cpu;
	unsigned long platid_machine;

	long timezone;
};

#define BI_CNUSE_BUILTIN	(1<<0)
#define BI_CNUSE_SERIAL		(1<<1)

extern struct bootinfo *bootinfo;
#define	BOOTINFO_MAGIC	0x13536135

#define BIFB_D2_M2L_3		0
#define BIFBN_D2_M2L_3		"D2_M2L_3"

#define BIFB_D2_M2L_3x2		1
#define BIFBN_D2_M2L_3x2	"D2_M2L_3x2"

#define BIFB_D2_M2L_0		2
#define BIFBN_D2_M2L_0		"D2_M2L_0"

#define BIFB_D8_00		3
#define BIFBN_D8_00		"D8_00"

#define BIFB_D8_FF		4
#define BIFBN_D8_FF		"D8_FF"

#define BIFB_D16_0000		5
#define BIFBN_D16_0000		"D16_0000"

#define BIFB_D16_FFFF		6
#define BIFBN_D16_FFFF		"D16_FFFF"

#define BIFB_D2_M2L_0x2		7
#define BIFBN_D2_M2L_0x2	"D2_M2L_0x2"

#define BIFB_D4_M2L_F		8
#define BIFBN_D4_M2L_F		"D4_M2L_F"

#define BIFB_D4_M2L_Fx2		9
#define BIFBN_D4_M2L_Fx2	"D4_M2L_Fx2"

#define BIFB_D4_M2L_0		10
#define BIFBN_D4_M2L_0		"D4_M2L_0"

#define BIFB_D4_M2L_0x2		11
#define BIFBN_D4_M2L_0x2	"D4_M2L_0x2"

#endif /* _HPC_BOOTINFO_H_ */
