/*-
 * Copyright (c) 2001 SATO Kazumi, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * definition for identify VR series cpu 
 * $NetBSD: vrcpudef.h,v 1.4 2002/02/10 15:09:32 sato Exp $
 *
 * REQUIRE #include "opt_vr41xx.h" before using this header.
 */
#include "opt_vr41xx.h"

/*
 * VR cpu id
 * It is related with BCU cpu id.
 * KEEP consistency with bcureg.h
 */
#define	VRID_4101	0x0		/* VR4101 */
#define	VRID_4102	0x1		/* VR4102 */
#define	VRID_4111	0x2		/* VR4111 */
#define	VRID_4121	0x3		/* VR4121 */
#define	VRID_4122	0x4		/* VR4122 */
#define	VRID_4131	0x5		/* VR4131 */
/* conflict other cpu */
#define	VRID_4181	0x10		/* VR4181 conflict VR4101 */

/*
 * VR cpu architecure group
 * 
 * vr41xx group:	all vr cpus (some registers are same)
 * vr4181 group:	vr4181 (vrip address and registers are same)
 * vr4101 group:	vr4101???
 * vr4102 group:	vr4102, vr4111, vr4121, vr4122 (some resgiters are same)
 *                or    vr4102, vr4111, vr4121 (vrip address and registers are same)
 * vr4111 group:	vr4111, vr4121, vr4122 (some registers are same)
 *                or	vr4111, vr4121 (vrip address and registers are same)
 * vr4122 group:	vr4122, vr4131 (vrip address and registers are same)
 *
 * REQUIRE #include "opt_vr41xx.h" before using this definition.
 */
#if defined VR4181
#define VRGROUP_4181		VRID_4181
#endif /* defined VR4181 */

#if defined VR4101
#define VRGROUP_4101		VRID_4101
#endif /* defined VR4101 */

#if defined VR4102
#define VRGROUP_4102		VRID_4102
#define VRGROUP_4102_4121	VRID_4102
#define VRGROUP_4102_4122	VRID_4102
#define VRGROUP_4102_4131	VRID_4102
#endif /* defined VR4102 */

#if defined VR4111
#define VRGROUP_4111_4121	VRID_4111
#define VRGROUP_4111_4122	VRID_4111
#define VRGROUP_4111_4131	VRID_4111

#ifndef VRGROUP_4102_4121
#define VRGROUP_4102_4121	VRID_4111
#endif /* VRGROUP_4102_4121 */

#ifndef VRGROUP_4102_4122
#define VRGROUP_4102_4122	VRID_4111
#endif /* VRGROUP_4102_4122 */

#ifndef VRGROUP_4102_4131
#define VRGROUP_4102_4131	VRID_4111
#endif /* VRGROUP_4102_4131 */

#endif /* defined VR4111 */

#if defined VR4121
#define VRGROUP_4121_4122	VRID_4121
#define VRGROUP_4121_4131	VRID_4131

#ifndef VRGROUP_4111_4121
#define VRGROUP_4111_4121	VRID_4121
#endif /* VRGROUP_4111_4121 */

#ifndef VRGROUP_4111_4122
#define VRGROUP_4111_4122	VRID_4121
#endif /* VRGROUP_4111_4122 */

#ifndef VRGROUP_4111_4131
#define VRGROUP_4111_4131	VRID_4121
#endif /* VRGROUP_4111_4131 */

#ifndef VRGROUP_4102_4121
#define VRGROUP_4102_4121	VRID_4121
#endif /* VRGROUP_4102_4121 */

#ifndef VRGROUP_4102_4122
#define VRGROUP_4102_4122	VRID_4121
#endif /* VRGROUP_4102_4122 */

#ifndef VRGROUP_4102_4131
#define VRGROUP_4102_4131	VRID_4121
#endif /* VRGROUP_4102_4131 */

#endif /* VR4121 */


#if defined VR4122
#define VRGROUP_4122		VRID_4122

#ifndef VRGROUP_4122_4131
#define VRGROUP_4122_4131	VRID_4122
#endif /* VRGROUP_4122_4131 */

#ifndef VRGROUP_4111_4122
#define VRGROUP_4111_4122	VRID_4122
#endif /* VRGROUP_4111_4122 */

#ifndef VRGROUP_4111_4131
#define VRGROUP_4111_4131	VRID_4122
#endif /* VRGROUP_4111_4131 */

#ifndef VRGROUP_4102_4122
#define VRGROUP_4102_4122	VRID_4122
#endif /* VRGROUP_4102_4122 */

#ifndef VRGROUP_4102_4131
#define VRGROUP_4102_4131	VRID_4122
#endif /* VRGROUP_4102_4131 */

#endif /* VR4122 */

#if defined VR4131
#define VRGROUP_4131		VRID_4131

#ifndef VRGROUP_4122_4131
#define VRGROUP_4122_4131	VRID_4131
#endif /* VRGROUP_4122_4131 */

#ifndef VRGROUP_4111_4131
#define VRGROUP_4111_4131	VRID_4131
#endif /* VRGROUP_4111_4131 */

#ifndef VRGROUP_4102_4131
#define VRGROUP_4102_4131	VRID_4131
#endif /* VRGROUP_4102_4131 */

#endif /* VR4131 */

/*
 * identify one cpu only
 */
#if defined VR4181 && !defined VR4101 && !defined VRGROUP_4102_4131
#define ONLY_VR4181	VRID_4181
#endif /* ONLY_VR4181 */

#if !defined VR4181 && defined VR4101 && !defined VRGROUP_4102_4131
#define ONLY_VR4101	VRID_4101
#endif /* ONLY_VR4101 */

#if !defined VRGROUP_4181 && !defined VRGROUP_4101 && defined VR4102 && !defined VRGROUP_4111_4131
#define ONLY_VR4102	VRID_4102
#endif /* ONLY_VR4102 */

#if !defined VRGROUP_4181 && !defined VRGROUP_4101 && !defined VR4102 && defined VRGROUP_4111_4121 && !defined VRGROUP_4122_4131
#define ONLY_VR4111_4121	VRGROUP_4111_4121
#endif /* ONLY_VR4111_4121 */

#if !defined VRGROUP_4181 && !defined VRGROUP_4101 && !defined VRGROUP_4102_4121 && defined VR4122 && !defined VRGROUP_4131
#define ONLY_VR4122	VRID4122
#endif /* ONLY_VR4122 */

#if !defined VRGROUP_4181 && !defined VRGROUP_4101 && !defined VRGROUP_4102_4121 && !defined VRGROUP_4122 && defined VR4131
#define ONLY_VR4131	VRID4131
#endif /* ONLY_VR4131 */

#if !defined VRGROUP_4181 && !defined VRGROUP_4101 && !defined VRGROUP_4102_4121 && defined VRGROUP_4122_4131
#define ONLY_VR4122_4131	VRGROUP_4122_4131
#endif /* ONLY_VR4131 */



#if defined ONLY_VR4181
#define ONLY_VR_SPECIFIED	ONLY_VR4181
#endif /* defined ONLY_VR4181 */

#if !defined ONLY_VR_SPECIFIED && defined ONLY_VR4101
#define ONLY_VR_SPECIFIED	ONLY_VR4101
#endif /* defined ONLY_VR4101 */

#if !defined ONLY_VR_SPECIFIED && defined ONLY_VR4102
#define ONLY_VR_SPECIFIED	ONLY_VR4102
#endif /* defined ONLY_VR4102 */

#if !defined ONLY_VR_SPECIFIED && defined ONLY_VR4111_4121
#define ONLY_VR_SPECIFIED	ONLY_VR4111_4121
#endif /* defined ONLY_VR4111_4121 */

#if !defined ONLY_VR_SPECIFIED && defined ONLY_VR4122_4131
#define ONLY_VR_SPECIFIED	ONLY_VR4122_4131
#endif /* ONLY_VR4122 */

#if !defined ONLY_VR_SPECIFIED && defined ONLY_VR4122
#define ONLY_VR_SPECIFIED	ONLY_VR4122
#endif /* ONLY_VR4122 */

#if !defined ONLY_VR_SPECIFIED && defined ONLY_VR4131
#define ONLY_VR_SPECIFIED	ONLY_VR4131
#endif /* ONLY_VR4131 */

/*
 * identify single vrip base address
 */
#if defined ONLY_VR4181
#define	SINGLE_VRIP_BASE	ONLY_VR4181
#endif /* defined ONLY_VR4181 */

#if !defined VR4181 && defined VRGROUP_4102_4121 && !defined VRGROUP_4122_4131
#define	SINGLE_VRIP_BASE	VRGROUP_4102_4121
#endif

#if !defined VR4181 && !defined VRGROUP_4102_4121 && defined VRGROUP_4122_4131
#define	SINGLE_VRIP_BASE	VRGROUP_4122_4131
#endif

/* end */
