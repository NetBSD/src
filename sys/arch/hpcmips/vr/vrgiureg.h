/*	$NetBSD: vrgiureg.h,v 1.2 2002/02/09 15:00:40 sato Exp $	*/

/*-
 * Copyright (c) 1999
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

/*
 *	VR GIU (General Purpose I/O Unit) Registers.
 */
#define GIU_NO_REG_W		0xffffffff	/* no register */

#define	GIUIOSEL_REG		0x00
#define	GIUIOSEL_L_REG_W	0x00
#define	GIUIOSEL_H_REG_W	0x02
#define GIUPIOD_REG		0x04
#define GIUPIOD_L_REG_W		0x04
#define GIUPIOD_H_REG_W		0x06
#define GIUINTSTAT_REG		0x08
#define GIUINTSTAT_L_REG_W	0x08
#define GIUINTSTAT_H_REG_W	0x0a
#define GIUINTEN_REG		0x0c
#define GIUINTEN_L_REG_W	0x0c
#define GIUINTEN_H_REG_W	0x0e
#define GIUINTTYP_REG		0x10
#define GIUINTTYP_L_REG_W	0x10
#define GIUINTTYP_H_REG_W	0x12
#define GIUINTALSEL_REG		0x14
#define GIUINTALSEL_L_REG_W	0x14
#define GIUINTALSEL_H_REG_W	0x16
#define GIUINTHTSEL_REG		0x18
#define GIUINTHTSEL_L_REG_W	0x18
#define GIUINTHTSEL_H_REG_W	0x1a
#define GIUPODAT_REG		0x1c
#define GIUPODAT_L_REG_W	0x1c
#define GIUPODAT_H_REG_W	0x1e

#define VR4102_GIUUSEUPDN_REG_W		0x1e0
#define VR4102_GIUTERMUPDN_REG_W	0x1e2

#define VR4122_GIUUSEUPDN_REG_W		GIU_NO_REG_W
#define VR4122_GIUTERMUPDN_REG_W	GIU_NO_REG_W

#if defined SINGLE_VRIP_BASE
#if defined VRGROUP_4102_4121
#define GIUUSEUPDN_REG_W	VR4102_GIUUSEUPDN_REG_W
#define GIUTERMUPDN_REG_W	VR4102_GIUTERMUPDN_REG_W
#endif /* defined VRGROUP_4102_4121 */
#if defined VRGROUP_4122_4131
#define GIUUSEUPDN_REG_W	VR4122_GIUUSEUPDN_REG_W
#define GIUTERMUPDN_REG_W	VR4122_GIUTERMUPDN_REG_W
#endif /* defined VRGROUP_4122_4131 */
#endif /* defined SINGLE_VRIP_BASE */

#define GIUINTTYP_EDGE		1
#define GIUINTTYP_LEVEL		0
#define GIUINTAL_HIGH		1
#define GIUINTAL_LOW		0
#define GIUINTHT_HOLD		1
#define GIUINTHT_THROUGH	0

/* END vrgiureg.h */
