/* $NetBSD: cpuconf.c,v 1.34.2.1 2012/04/17 00:05:53 yamt Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpuconf.c,v 1.34.2.1 2012/04/17 00:05:53 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <machine/cpuconf.h>
#include <machine/rpb.h>

#include "opt_dec_3000_500.h"
#ifdef DEC_3000_500
extern void dec_3000_500_init(void);
#else
#define	dec_3000_500_init	platform_not_configured
#endif

#include "opt_dec_3000_300.h"
#ifdef DEC_3000_300
extern void dec_3000_300_init(void);
#else
#define	dec_3000_300_init	platform_not_configured
#endif

#include "opt_dec_axppci_33.h"
#ifdef DEC_AXPPCI_33
extern void dec_axppci_33_init(void);
#else
#define	dec_axppci_33_init	platform_not_configured
#endif

#include "opt_dec_kn8ae.h"
#ifdef DEC_KN8AE
extern void dec_kn8ae_init(void);
#else
#define	dec_kn8ae_init		platform_not_configured
#endif

#include "opt_dec_2100_a50.h"
#ifdef DEC_2100_A50
extern void dec_2100_a50_init(void);
#else
#define	dec_2100_a50_init	platform_not_configured
#endif

#include "opt_dec_kn20aa.h"
#ifdef DEC_KN20AA
extern void dec_kn20aa_init(void);
#else
#define	dec_kn20aa_init		platform_not_configured
#endif

#include "opt_dec_eb64plus.h"
#ifdef DEC_EB64PLUS
extern void dec_eb64plus_init(void);
#else
#define	dec_eb64plus_init	platform_not_configured
#endif

#include "opt_dec_eb164.h"
#ifdef DEC_EB164
extern void dec_eb164_init(void);
#else
#define	dec_eb164_init		platform_not_configured
#endif

#include "opt_dec_kn300.h"
#ifdef	DEC_KN300
extern void dec_kn300_init(void);
#else
#define	dec_kn300_init		platform_not_configured
#endif

#include "opt_dec_550.h"
#ifdef DEC_550
extern void dec_550_init(void);
#else
#define	dec_550_init		platform_not_configured
#endif

#include "opt_dec_1000.h"
#include "opt_dec_1000a.h"
#if defined(DEC_1000) || defined(DEC_1000A)
extern void _dec_1000a_init(void);
#endif
#ifdef DEC_1000A
#define	dec_1000a_init		_dec_1000a_init
#else
#define	dec_1000a_init		platform_not_configured
#endif
#ifdef DEC_1000
#define	dec_1000_init		_dec_1000a_init
#else
#define	dec_1000_init		platform_not_configured
#endif

#include "opt_dec_alphabook1.h"
#ifdef DEC_ALPHABOOK1
extern void dec_alphabook1_init(void);
#else
#define	dec_alphabook1_init	platform_not_configured
#endif

#include "opt_dec_eb66.h"
#ifdef DEC_EB66
extern void dec_eb66_init(void);
#else
#define	dec_eb66_init		platform_not_configured
#endif

#include "opt_dec_6600.h"
#ifdef DEC_6600
extern void dec_6600_init(void);
#else
#define	dec_6600_init		platform_not_configured
#endif

#include "opt_dec_2100_a500.h"
#include "opt_dec_2100a_a500.h"
#if defined(DEC_2100_A500) || defined(DEC_2100A_A500)
extern void _dec_2100_a500_init(void);
#endif
#ifdef DEC_2100_A500
#define	dec_2100_a500_init	_dec_2100_a500_init
#else
#define	dec_2100_a500_init	platform_not_configured
#endif
#ifdef DEC_2100A_A500
#define	dec_2100a_a500_init	_dec_2100_a500_init
#else
#define	dec_2100a_a500_init	platform_not_configured
#endif

#include "opt_api_up1000.h"
#ifdef API_UP1000
extern void api_up1000_init(void);
#else
#define	api_up1000_init		platform_not_configured
#endif

#include "opt_dec_2000_300.h"
#ifdef DEC_2000_300
extern void dec_2000_300_init(void);
#else
#define	dec_2000_300_init	platform_not_configured
#endif

static const struct cpuinit cpuinit[] = {
	cpu_notsupp(ST_ADU, "Alpha Demo Unit"),
	cpu_notsupp(ST_DEC_4000, "DEC 4000 (``Cobra'')"),
	cpu_notsupp(ST_DEC_7000, "DEC 7000 (``Ruby'')"),
	cpu_init(ST_DEC_3000_500, dec_3000_500_init, "DEC_3000_500"),
	cpu_init(ST_DEC_2000_300, dec_2000_300_init, "DEC_2000_300"),
	cpu_init(ST_DEC_3000_300, dec_3000_300_init, "DEC_3000_300"),
	cpu_init(ST_DEC_2100_A500, dec_2100_a500_init, "DEC_2100_A500"),
	cpu_notsupp(ST_DEC_APXVME_64, "AXPvme 64"),
	cpu_init(ST_DEC_AXPPCI_33, dec_axppci_33_init, "DEC_AXPPCI_33"),
	cpu_init(ST_DEC_21000, dec_kn8ae_init, "DEC_KN8AE"),
	cpu_init(ST_DEC_2100_A50, dec_2100_a50_init, "DEC_2100_A50"),
	cpu_notsupp(ST_DEC_MUSTANG, "Mustang"),
	cpu_init(ST_DEC_KN20AA, dec_kn20aa_init, "DEC_KN20AA"),
	cpu_init(ST_DEC_1000, dec_1000_init, "DEC_1000"),
	cpu_init(ST_EB66, dec_eb66_init, "DEC_EB66"),
	cpu_init(ST_EB64P, dec_eb64plus_init, "DEC_EB64PLUS"),
	cpu_init(ST_ALPHABOOK1, dec_alphabook1_init, "DEC_ALPHABOOK1"),
	cpu_init(ST_DEC_4100, dec_kn300_init, "DEC_KN300"),
	cpu_notsupp(ST_DEC_EV45_PBP, "EV45 Passive Backplane Board"),
	cpu_init(ST_DEC_2100A_A500, dec_2100a_a500_init, "DEC_2100A_A500"),
	cpu_init(ST_EB164, dec_eb164_init, "DEC_EB164"),
	cpu_init(ST_DEC_1000A, dec_1000a_init, "DEC_1000A"),
	cpu_notsupp(ST_DEC_ALPHAVME_224, "AlphaVME 224"),
	cpu_init(ST_DEC_550, dec_550_init, "DEC_550"),
	cpu_notsupp(ST_DEC_EV56_PBP, "EV56 Passive Backplane Board"),
	cpu_notsupp(ST_DEC_ALPHAVME_320, "AlphaVME 320"),
	cpu_init(ST_DEC_6600, dec_6600_init, "DEC_6600"),
	cpu_init(ST_API_NAUTILUS, api_up1000_init, "API_UP1000"),
};
static const int ncpuinit = (sizeof(cpuinit) / sizeof(cpuinit[0]));

const struct cpuinit *
platform_lookup(int systype)
{
	const struct cpuinit *c;
	int i;

	for (i = 0; i < ncpuinit; i++) {
		c = &cpuinit[i];
		if (c->systype == systype)
			return (c);
	}
	return (NULL);
}

void
platform_not_configured(void)
{
	const struct cpuinit *c = platform_lookup(cputype);

	printf("\n");
	printf("Support for system type %d is not present in this kernel.\n",
	    cputype);
	printf("Please build a kernel with \"options %s\" and reboot.\n",
	    c->option);
	printf("\n");
	panic("platform not configured");
}

void
platform_not_supported(void)
{
	const struct cpuinit *c = platform_lookup(cputype);

	printf("\n");
	printf("NetBSD does not yet support system type %d (%s).\n", cputype,
	    (c != NULL) ? c->option : "???");
	printf("\n");
	panic("platform not supported");
}
