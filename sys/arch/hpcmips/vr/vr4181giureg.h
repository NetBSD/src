/*	$NetBSD: vr4181giureg.h,v 1.1.2.2 2002/02/28 04:10:05 nathanw Exp $	*/

/*-
 * Copyright (c) 2002 SATO Kazumi. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
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

/*
 *	VR4181 GIU (General Purpose I/O Unit) Registers.
 */
#define VR4181GIU_NO_REG_W		0xffffffff	/* no register */

#define VR4181GIU_MODE0_REG		0x00
#define VR4181GIU_MODE0_REG_W		0x00
#define VR4181GIU_MODE1_REG_W		0x02
#define VR4181GIU_MODE2_REG_W		0x04
#define VR4181GIU_MODE3_REG_W		0x06

#define	VR4181GIU_MODE_GPIO		0x0
#define	VR4181GIU_MODE_ALT		0x1
/* VR4181GIU_MODE_GPIO */
#define	VR4181GIU_MODE_IN		0x0
#define	VR4181GIU_MODE_OUT		0x2
/* VR4181GIU_MODE_ALT */
#define	VR4181GIU_MODE_ALT1		0x0
#define	VR4181GIU_MODE_ALT2		0x2

#define VR4181GIU_PIOD_REG		0x08
#define VR4181GIU_PIOD_L_REG_W		0x08
#define VR4181GIU_PIOD_H_REG_W		0x0a

#define VR4181GIU_INTEN_REG_W		0x0c

#define VR4181GIU_INTMASK_REG_W	0x0e

#define VR4181GIU_INTTYP_REG		0x10
#define VR4181GIU_INTTYP_L_REG_W	0x10
#define VR4181GIU_INTTYP_H_REG_W	0x12

#define VR4181GIU_INTSTAT_REG_W	0x14

#define VR4181GIU_HIBST_REG		0x16
#define VR4181GIU_HIBST_L_REG_W	0x16
#define VR4181GIU_HIBST_H_REG_W	0x18

#define VR4181GIU_SICTL_REG_W		0x1a

#define VR4181GIU_KEYEN_REG_W		0x1c

#define VR4181GIU_INTTYP_EDGE		0		
#define VR4181GIU_INTTYP_LEVEL		0x2
#define VR4181GIU_INTTYP_HIGH		1	
#define VR4181GIU_INTTYP_LOW		0

/* END vr4181giu.h */
