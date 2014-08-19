/*	$NetBSD: locore.c,v 1.80.12.1 2014/08/20 00:03:27 tls Exp $	*/
/*
 * Copyright (c) 1994, 1998 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
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

 /* All bugs are subject to removal without further notice */
		
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: locore.c,v 1.80.12.1 2014/08/20 00:03:27 tls Exp $");

#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/reboot.h>

#include <uvm/uvm_extern.h>

#include <machine/sid.h>
#include <machine/nexus.h>
#include <machine/rpb.h>

#include "opt_cputype.h"

void	_start(struct rpb *);
void	main(void);

extern	paddr_t avail_end;
paddr_t esym;

/*
 * The strict CPU-dependent information is set up here, in
 * form of a pointer to a struct that is specific for each CPU.
 */
extern const struct cpu_dep ka780_calls;
extern const struct cpu_dep ka750_calls;
extern const struct cpu_dep ka730_calls;
extern const struct cpu_dep ka860_calls;
extern const struct cpu_dep ka820_calls;
extern const struct cpu_dep ka6400_calls;
extern const struct cpu_dep ka88_calls;
extern const struct cpu_dep ka43_calls;
extern const struct cpu_dep ka46_calls;
extern const struct cpu_dep ka48_calls;
extern const struct cpu_dep vxt_calls;
extern const struct cpu_dep ka49_calls;
extern const struct cpu_dep ka53_calls;
extern const struct cpu_dep ka410_calls;
extern const struct cpu_dep ka610_calls;
extern const struct cpu_dep ka630_calls;
extern const struct cpu_dep ka650_calls;
extern const struct cpu_dep ka660_calls;
extern const struct cpu_dep ka670_calls;
extern const struct cpu_dep ka680_calls;

/*
 * Start is called from boot; the first routine that is called
 * in kernel. Kernel stack is setup somewhere in a safe place;
 * but we need to move it to a better known place. Memory
 * management is disabled, and no interrupt system is active.
 */
void
_start(struct rpb *prpb)
{
	extern uintptr_t scratch;
	struct pte *pt;
	vaddr_t uv;
	const char *mv;
#if VAX410 || VAXANY
	const char *md;
#endif

	mtpr(AST_NO, PR_ASTLVL); /* Turn off ASTs */

	findcpu(); /* Set up the CPU identifying variables */

	if (vax_confdata & 0x80)
		mv = "MicroVAX";
	else
		mv = "VAXstation";

	switch (vax_boardtype) {
#if VAX780 || VAXANY
	case VAX_BTYP_780:
		dep_call = &ka780_calls;
		cpu_setmodel("VAX 11/78%c", vax_cpudata & 0x100 ? '5' : '0');
		mv = NULL;
		break;
#endif
#if VAX750 || VAXANY
	case VAX_BTYP_750:
		dep_call = &ka750_calls;
		mv = "VAX 11/750";
		break;
#endif
#if VAX730 || VAXANY
	case VAX_BTYP_730:
		dep_call = &ka730_calls;
		mv = "VAX 11/730";
		break;
#endif
#if VAX8600 || VAXANY
	case VAX_BTYP_790:
		dep_call = &ka860_calls;
		cpu_setmodel("VAX 86%c0", vax_cpudata & 0x800000 ? '5' : '0');
		mv = NULL;
		break;
#endif
#if VAX410 || VAXANY
	case VAX_BTYP_420: /* They are very similar */
		dep_call = &ka410_calls;
		if (((vax_siedata >> 8) & 0xff) == 1)
			md = "/m{38,48}";
		else if (((vax_siedata >> 8) & 0xff) == 0)
			md = "/m{30,40}";
		else
			md = "";
		cpu_setmodel("%s 3100%s", mv, md);
		mv = NULL;
		break;

	case VAX_BTYP_410:
		dep_call = &ka410_calls;
		cpu_setmodel("%s 2000", mv);
		mv = NULL;
		break;
#endif
#if VAX43 || VAXANY
	case VAX_BTYP_43:
		dep_call = &ka43_calls;
		cpu_setmodel("%s 3100/m76", mv);
		mv = NULL;
		break;
#endif
#if VAX46 || VAXANY
	case VAX_BTYP_46:
		dep_call = &ka46_calls;
		switch(vax_siedata & 0x3) {
		case 1: mv = "MicroVAX 3100/80"; break;
		case 2: mv = "VAXstation 4000/60"; break;
		default: mv = "unknown"; break;
		}
		break;
#endif
#if VAX48 || VAXANY
	case VAX_BTYP_48:
		dep_call = &ka48_calls;
		switch (vax_siedata & 3) {
		case 1: mv = "MicroVAX 3100/m{30,40}"; break;
		case 2: mv = "VAXstation 4000 VLC"; break;
		default: mv = "unknown SOC"; break;
		}
		break;
#endif
#if 0 && (VXT2000 || VAXANY)
	case VAX_BTYP_VXT:
		dep_call = &vxt_calls;
		mv = "VXT 2000 X terminal";
		break;
#endif
#if VAX49 || VAXANY
	case VAX_BTYP_49:
		dep_call = &ka49_calls;
		cpu_setmodel("%s 4000/90", mv);
		mv = NULL;
		break;
#endif
#if VAX53 || VAXANY
	case VAX_BTYP_53:
		dep_call = &ka53_calls;
		switch((vax_siedata & 0xff00) >> 8) {
		case VAX_STYP_51:
			mv = "MicroVAX 3100/m{90,95}"; break;
		case VAX_STYP_52:
			mv = "VAX 4000/100"; break;
		case VAX_STYP_53:
			mv = "VAX 4000/{105A,106A,108}"; break;
		case VAX_STYP_55:
			mv = "MicroVAX 3100/m85"; break;
		default:
			mv = "unknown 1303"; break;
		}
		break;
#endif
#if VAX610 || VAXANY
	case VAX_BTYP_610:
		dep_call = &ka610_calls;
		mv = "MicroVAX I";
		break;
#endif
#if VAX630 || VAXANY
	case VAX_BTYP_630:
		dep_call = &ka630_calls;
		mv = "MicroVAX II";
		break;
#endif
#if VAX650 || VAXANY
	case VAX_BTYP_650:
		dep_call = &ka650_calls;
		switch ((vax_siedata >> 8) & 255) {
		case VAX_SIE_KA640:
			mv = "3300/3400";
			break;

		case VAX_SIE_KA650:
			mv = "3500/3600";
			break;

		case VAX_SIE_KA655:
			mv = "3800/3900";
			break;

		default:
			mv = "III";
			break;
		}
		cpu_setmodel("MicroVAX %s", mv);
		mv = NULL;
		break;
#endif
#if VAX660 || VAXANY
	case VAX_BTYP_660:
		dep_call = &ka660_calls;
		mv = "VAX 4000/200";
		break;
#endif
#if VAX670 || VAXANY
	case VAX_BTYP_670:
		dep_call = &ka670_calls;
		mv = "VAX 4000/300";
		break;
#endif
#if VAX680 || VAXANY
	case VAX_BTYP_680:
		dep_call = &ka680_calls;
		switch((vax_siedata & 0xff00) >> 8) {
		case VAX_STYP_675:
			mv = "VAX 4000/400"; break;
		case VAX_STYP_680:
			mv = "VAX 4000/500"; break;
		default:
			mv = "unknown 1301";
		}
		break;
	case VAX_BTYP_681:
		dep_call = &ka680_calls;
		switch((vax_siedata & 0xff00) >> 8) {
		case VAX_STYP_681:
			mv = "VAX 4000/500A"; break;
		case VAX_STYP_691:
			mv = "VAX 4000/600A"; break;
		case VAX_STYP_694:
			cpu_setmodel("VAX 4000/70%cA",
			    vax_cpudata & 0x1000 ? '5' : '0');
			mv = NULL;
			break;
		default:
			mv = "unknown 1305";
		}
		break;
#endif
#if VAX8200 || VAXANY
	case VAX_BTYP_8000:
		dep_call = &ka820_calls;
		mv = "VAX 8200";
		break;
#endif
#if VAX8800 || VAXANY
	case VAX_BTYP_8PS:
	case VAX_BTYP_8800: /* Matches all other KA88-machines also */
		mv = "VAX 8800";
		dep_call = &ka88_calls;
		break;
#endif
#if VAX6400 || VAXANY
	case VAX_BTYP_9RR:
		/* XXX (a lie): cpu model is not set in steal_pages */
		dep_call = &ka6400_calls;
		break;
#endif
	default:
		/* CPU not supported, just give up */
		__asm("halt");
	}

	if (mv != NULL)
		cpu_setmodel("%s", mv);

	/*
	 * Machines older than MicroVAX II have their boot blocks
	 * loaded directly or the boot program loaded from console
	 * media, so we need to figure out their memory size.
	 * This is not easily done on MicroVAXen, so we get it from
	 * VMB instead.
	 *
	 * In post-1.4 a RPB is always provided from the boot blocks.
	 */
	uv = uvm_lwp_getuarea(&lwp0);
	uv += REDZONEADDR;
#if defined(COMPAT_14)
	if (prpb == 0) {
		memset((void *)uv, 0, sizeof(struct rpb));
		prpb = (struct rpb *)uv;
		prpb->pfncnt = avail_end >> VAX_PGSHIFT;
		prpb->rpb_base = (void *)-1;	/* RPB is fake */
	} else
#endif
	memcpy((void *)uv, prpb, sizeof(struct rpb));
	if (prpb->pfncnt)
		avail_end = prpb->pfncnt << VAX_PGSHIFT;
	else
		while (badaddr((void *)avail_end, 4) == 0)
			avail_end += VAX_NBPG * 128;
	boothowto = prpb->rpb_bootr5;

	avail_end &= ~PGOFSET; /* be sure */

	pmap_bootstrap();

	/* Now running virtual. set red zone for proc0 */
	pt = kvtopte(uv);
	pt->pg_v = 0;

	lwp0.l_md.md_utf = (void *)scratch;

	/*
	 * Change mode down to userspace is done by faking a stack
	 * frame that is setup in cpu_set_kpc(). Not done by returning
	 * from main anymore.
	 */
	main();
	/* NOTREACHED */
}
