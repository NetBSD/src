/*	$NetBSD: machdep.h,v 1.3 1999/04/26 09:23:23 nisimura Exp $	*/

/*
 * Copyright (c) 1998 Jonathan Stone.  All rights reserved.
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
 *	This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
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

/* the following is used externally (sysctl_hw) */
extern char machine[];
extern char cpu_model[];

/*
 * Interrupt-blocking functions defined in locore. These names aren't used
 * directly except here and in interrupt handlers.
 */

/* Block out one hardware interrupt-enable bit. */
extern int	Mach_spl0 __P((void)), Mach_spl1 __P((void));
extern int	Mach_spl2 __P((void)), Mach_spl3 __P((void));

/* Block out nested interrupt-enable bits. */
extern int	cpu_spl0 __P((void)), cpu_spl1 __P((void));
extern int	cpu_spl2 __P((void)), cpu_spl3 __P((void));
extern int	splhigh __P((void));

extern volatile struct chiptime *mcclock_addr;


/* jump to PROM after halt switch.  */
extern void prom_haltbutton __P((void));


/* high-res clock ticks.  Need better timer support. */
u_long latched_cycle_cnt;

/* XXX max memory */
int	physmem_boardmax;	/* {model,simm}-specific bound on physmem */
