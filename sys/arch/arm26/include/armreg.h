/* $NetBSD: armreg.h,v 1.1 2000/05/09 21:55:58 bjh21 Exp $ */
/*-
 * Copyright (c) 1998 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */

#ifndef _ARM26_ARMREG_H
#define _ARM26_ARMREG_H

/* All the junk the old ARMs stuff in R15 */

#define R15_MODE	0x00000003
#define R15_MODE_USR	0x00000000
#define R15_MODE_FIQ	0x00000001
#define R15_MODE_IRQ	0x00000002
#define R15_MODE_SVC	0x00000003

#define R15_PC		0x03fffffc

#define R15_FIQ_DISABLE	0x04000000
#define R15_IRQ_DISABLE	0x08000000

#define R15_FLAGS	0xf0000000
#define R15_FLAG_N	0x80000000
#define R15_FLAG_Z	0x40000000
#define R15_FLAG_C	0x20000000
#define R15_FLAG_V	0x10000000

#define ARM_CP15_CPU_ID		0

/* Not really many CPUs to consider */
#define CPU_ID_DESIGNER_MASK	0xff000000
#define CPU_ID_ARM_LTD		0x41000000
#define CPU_ID_TYPE_MASK	0x00ff0000
#define CPU_ID_ARM		0x00560000
#define CPU_ID_CPU_MASK		0x0000fff0
#define CPU_ID_ARM3		0x00000300
#define CPU_ID_REVISION_MASK	0x0000000f

/* Fake CPU IDs for older ARMs */
#define CPU_ID_ARM2	0x00000200
#define CPU_ID_ARM250	0x00000250

/* ARM3-specific coprocessor 15 register */
#define ARM3_CP15_FLUSH		1
#define ARM3_CP15_CONTROL	2
#define ARM3_CP15_CACHEABLE	3
#define ARM3_CP15_UPDATEABLE	4
#define ARM3_CP15_DISRUPTIVE	5	

/* Control register bits */
#define ARM3_CTL_CACHE_ON	1
#define ARM3_CTL_SHARED		2
#define ARM3_CTL_MONITOR	4

#endif
