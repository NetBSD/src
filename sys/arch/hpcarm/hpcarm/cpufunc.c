/*	$NetBSD: cpufunc.c,v 1.3.2.2 2001/03/12 13:28:20 bouyer Exp $	*/

/*
 * arm8 support code Copyright (c) 1997 ARM Limited
 * arm8 support code Copyright (c) 1997 Causality Limited
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Causality Limited.
 * 4. The name of Causality Limited may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CAUSALITY LIMITED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CAUSALITY LIMITED BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * cpufuncs.c
 *
 * C functions for supporting CPU / MMU / TLB specific operations.
 *
 * Created      : 30/01/97
 */

#include "opt_compat_netbsd.h"
#include "opt_pmap_debug.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/bootconfig.h>
#include <arch/arm/arm/disassem.h>

#ifdef CPU_SA110
struct cpu_functions sa110_cpufuncs = {
	/* CPU functions */
	
	cpufunc_id,			/* id			 */

	/* MMU functions */

	cpufunc_control,		/* control		*/
	cpufunc_domains,		/* domain		*/
	sa110_setttb,			/* setttb		*/
	cpufunc_faultstatus,		/* faultstatus		*/
	cpufunc_faultaddress,		/* faultaddress		*/

	/* TLB functions */

	sa110_tlb_flushID,		/* tlb_flushID		*/
	sa110_tlb_flushID_SE,		/* tlb_flushID_SE		*/
	sa110_tlb_flushI,		/* tlb_flushI		*/
	(void *)sa110_tlb_flushI,	/* tlb_flushI_SE	*/
	sa110_tlb_flushD,		/* tlb_flushD		*/
	sa110_tlb_flushD_SE,		/* tlb_flushD_SE	*/

	/* Cache functions */

	sa110_cache_flushID,		/* cache_flushID	*/
	(void *)sa110_cache_flushID,	/* cache_flushID_SE	*/
	sa110_cache_flushI,		/* cache_flushI		*/
	(void *)sa110_cache_flushI,	/* cache_flushI_SE	*/
	sa110_cache_flushD,		/* cache_flushD		*/
	sa110_cache_flushD_SE,		/* cache_flushD_SE	*/

	sa110_cache_cleanID,		/* cache_cleanID	s*/
	sa110_cache_cleanD_E,		/* cache_cleanID_E	s*/
	sa110_cache_cleanD,		/* cache_cleanD		s*/
	sa110_cache_cleanD_E,		/* cache_cleanD_E	*/

	sa110_cache_purgeID,		/* cache_purgeID	s*/
	sa110_cache_purgeID_E,		/* cache_purgeID_E	s*/
	sa110_cache_purgeD,		/* cache_purgeD		s*/
	sa110_cache_purgeD_E,		/* cache_purgeD_E	s*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	sa110_drain_writebuf,		/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	(void *)cpufunc_nullop,		/* sleep		*/

	/* Soft functions */

	sa110_cache_syncI,		/* cache_syncI		*/
	sa110_cache_cleanID_rng,	/* cache_cleanID_rng	*/
	sa110_cache_cleanD_rng,		/* cache_cleanD_rng	*/
	sa110_cache_purgeID_rng,	/* cache_purgeID_rng	*/
	sa110_cache_purgeD_rng,		/* cache_purgeD_rng	*/
	sa110_cache_syncI_rng,		/* cache_syncI_rng	*/

	cpufunc_null_fixup,		/* dataabt_fixup	*/
	cpufunc_null_fixup,	/* prefetchabt_fixup	*/

	sa110_context_switch,		/* context_switch	*/

	sa110_setup			/* cpu setup		*/
};          
#endif	/* CPU_SA110 */

struct cpu_functions cpufuncs;
u_int cputype;

/*
 * Cannot panic here as we may not have a console yet ...
 */

int
set_cpufuncs()
{
	cputype = cpufunc_id();
	cputype &= CPU_ID_CPU_MASK;

	switch (cputype) {

#if defined(CPU_SA110) || defined(CPU_SA1110)
	case CPU_ID_SA110:
	case CPU_ID_SA1110:
		cpufuncs = sa110_cpufuncs;
		break;
#endif	/* CPU_SA110 */
	default:
		/*
		 * Bzzzz. And the answer was ...
		 */
/*		panic("No support for this CPU type (%03x) in kernel\n", id >> 4);*/
		return(ARCHITECTURE_NOT_PRESENT);
		break;
	}
	return(0);
}

/*
 * Fixup routines for data and prefetch aborts.
 *
 * Several compile time symbols are used
 *
 * DEBUG_FAULT_CORRECTION - Print debugging information during the
 * correction of registers after a fault.
 * ARM6_LATE_ABORT - ARM6 supports both early and late aborts
 * when defined should use late aborts
 */

#if defined(DEBUG_FAULT_CORRECTION) && !defined(PMAP_DEBUG)
#error PMAP_DEBUG must be defined to use DEBUG_FAULT_CORRECTION
#endif

/*
 * Null abort fixup routine.
 * For use when no fixup is required.
 */
int
cpufunc_null_fixup(arg)
	void *arg;
{
	return(ABORT_FIXUP_OK);
}


/*
 * CPU Setup code
 */

int cpuctrl;

#define IGN	0
#define OR	1
#define BIC	2

struct cpu_option {
	char	*co_name;
	int	co_falseop;
	int	co_trueop;
	int	co_value;
};

static u_int
parse_cpu_options(args, optlist, cpuctrl)
	char *args;
	struct cpu_option *optlist;    
	u_int cpuctrl; 
{
	int integer;

	while (optlist->co_name) {
		if (get_bootconf_option(args, optlist->co_name,
		    BOOTOPT_TYPE_BOOLEAN, &integer)) {
			if (integer) {
				if (optlist->co_trueop == OR)
					cpuctrl |= optlist->co_value;
				else if (optlist->co_trueop == BIC)
					cpuctrl &= ~optlist->co_value;
			} else {
				if (optlist->co_falseop == OR)
					cpuctrl |= optlist->co_value;
				else if (optlist->co_falseop == BIC)
					cpuctrl &= ~optlist->co_value;
			}
		}
		++optlist;
	}
	return(cpuctrl);
}

#ifdef CPU_SA110
struct cpu_option sa110_options[] = {
#ifdef COMPAT_12
	{ "nocache",		IGN, BIC, (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "nowritebuf",		IGN, BIC, CPU_CONTROL_WBUF_ENABLE },
#endif	/* COMPAT_12 */
	{ "cpu.cache",		BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "cpu.nocache",	OR,  BIC, (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "sa110.cache",	BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "sa110.icache",	BIC, OR,  CPU_CONTROL_IC_ENABLE },
	{ "sa110.dcache",	BIC, OR,  CPU_CONTROL_DC_ENABLE },
	{ "cpu.writebuf",	BIC, OR,  CPU_CONTROL_WBUF_ENABLE },
	{ "cpu.nowritebuf",	OR,  BIC, CPU_CONTROL_WBUF_ENABLE },
	{ "sa110.writebuf",	BIC, OR,  CPU_CONTROL_WBUF_ENABLE },
	{ NULL,			IGN, IGN, 0 }
};

void
sa110_setup(args)
	char *args;
{
	int cpuctrlmask;

	cpuctrl = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
		 | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
		 | CPU_CONTROL_WBUF_ENABLE;
	cpuctrlmask = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
		 | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
		 | CPU_CONTROL_WBUF_ENABLE | CPU_CONTROL_ROM_ENABLE
		 | CPU_CONTROL_BEND_ENABLE | CPU_CONTROL_AFLT_ENABLE
		 | CPU_CONTROL_LABT_ENABLE | CPU_CONTROL_BPRD_ENABLE
		 | CPU_CONTROL_CPCLK;

	cpuctrl = parse_cpu_options(args, sa110_options, cpuctrl);

	/* Clear out the cache */
	cpu_cache_purgeID();

	/* Set the control register */    
/*	cpu_control(cpuctrlmask, cpuctrl);*/
	cpu_control(0xffffffff, cpuctrl);
}
#endif	/* CPU_SA110 */
