/*	$NetBSD: pmc.h,v 1.1.152.1 2014/08/20 00:02:42 tls Exp $	*/

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

#ifdef _KERNEL
/*
 * LEGACY PMC support
 */
struct x86_64_pmc_info_args;
int	pmc_info(struct proc *, struct x86_64_pmc_info_args *,
	    register_t *);
struct x86_64_pmc_startstop_args;
int	pmc_startstop(struct proc *, struct x86_64_pmc_startstop_args *,
	    register_t *);
struct x86_64_pmc_read_args;
int	pmc_read(struct proc *, struct x86_64_pmc_read_args *,
	    register_t *);
/* END LEGACY PMC SUPPORT */

#define pmc_md_fork(p1,p2)
#define pmc_get_num_counters()			(0)
#define pmc_get_counter_type(c)			(0)
#define pmc_save_context(p)
#define pmc_restore_context(p)
#define pmc_enable_counter(p,c)
#define pmc_disable_counter(p,c)
#define pmc_accumulate(p1,p2)
#define pmc_process_exit(p1)
#define pmc_counter_isconfigured(p,c)		(0)
#define pmc_counter_isrunning(p,c)		(0)
#define pmc_start_profiling(c,f)		(0)
#define pmc_stop_profiling(c)			(0)
#define pmc_alloc_kernel_counter(c,f)		(0)
#define pmc_free_kernel_counter(c)		(0)
#define pmc_configure_counter(p,c,f)		(0)
#define pmc_get_counter_value(p,c,f,pv)		(0)

#define PMC_ENABLED(p)		(0)

#endif
