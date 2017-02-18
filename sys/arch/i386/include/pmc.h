/*	$NetBSD: pmc.h,v 1.10 2017/02/18 14:36:32 maxv Exp $	*/

/*-
 * Copyright (c) 2000 Zembu Labs, Inc.
 * All rights reserved.
 *
 * Author: Jason R. Thorpe <thorpej@zembu.com>
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
 *	This product includes software developed by Zembu Labs, Inc.
 * 4. Neither the name of Zembu Labs nor the names of its employees may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ZEMBU LABS, INC. ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAR-
 * RANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DIS-
 * CLAIMED.  IN NO EVENT SHALL ZEMBU LABS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _I386_PMC_H_
#define	_I386_PMC_H_

#define	PMC_CLASS_I586		0x10000		/* i586-compatible */
#define	PMC_TYPE_I586_TSC	0x10001		/* cycle counter */
#define	PMC_TYPE_I586_PMCx	0x10002		/* performance counter */

#define	PMC_CLASS_I686		0x20000		/* i686-compatible */
#define	PMC_TYPE_I686_TSC	0x20001		/* cycle counter */
#define	PMC_TYPE_I686_PMCx	0x20002		/* performance counter */

#define	PMC_CLASS_K7		0x30000		/* K7-compatible */
#define	PMC_TYPE_K7_TSC		0x30001		/* cycle counter */
#define	PMC_TYPE_K7_PMCx	0x30002		/* performance counter */

/*
 * Each PMC event on the x86 is associated with a processor unit.  We
 * encode the unit in the upper 16 bits of the event ID.
 */
#define	__PMC_UNIT(x)		((x) << 16)

#if defined(_KERNEL)
struct x86_pmc_info_args;
struct x86_pmc_startstop_args;
struct x86_pmc_read_args;
void pmc_init(void);
int sys_pmc_info(struct lwp *, struct x86_pmc_info_args *,
    register_t *);
int sys_pmc_startstop(struct lwp *, struct x86_pmc_startstop_args *,
    register_t *);
int sys_pmc_read(struct lwp *, struct x86_pmc_read_args *,
    register_t *);
#endif /* _KERNEL */

#endif /* _I386_PMC_H_ */
