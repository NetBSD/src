/*	$NetBSD: cputypes.h,v 1.1 1999/09/13 10:31:16 itojun Exp $	*/

/*
 * Copyright (c) 1993 Christopher G. Demetriou
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
 *    derived from this software without specific prior written permission
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
 * Classes of Processor. CPU identification code depends on
 * this starting at 0, and having an increment of one.
 */

#define CPUCLASS_SH3	0

/*
 * Kinds of Processor. Only the first 1 are used, as they are processors
 * that might not have a cpuid instruction.
 */

#define CPU_SH3		0	/* Hitachi SH3 */

/*
 * CPU vendors
 */

#define CPUVENDOR_UNKNOWN	-1
#define CPUVENDOR_HITACHI		0

/*
 * Some other defines, dealing with values returned by cpuid.
 */

#if 0
#define CPU_MAXMODEL	15	/* Models within family range 0-15 */
#define CPU_DEFMODEL	16	/* Value for unknown model -> default  */
#define CPU_MINFAMILY	 4	/* Lowest that cpuid can return (486) */
#define CPU_MAXFAMILY	 6	/* Highest we know (686) */
#endif
