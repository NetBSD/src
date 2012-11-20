/*	$NetBSD: fpu.c,v 1.13.12.1 2012/11/20 03:00:53 tls Exp $	*/

/*-
 * Copyright (c) 2000, 2001 Ben Harris
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
/*
 * fpu.c - Floating point unit support
 */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: fpu.c,v 1.13.12.1 2012/11/20 03:00:53 tls Exp $");

#include <sys/device.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <arm/undefined.h>
#include <machine/fpureg.h>
#include <machine/pcb.h>


#include <arch/acorn26/acorn26/fpuvar.h>

#include "opt_fputypes.h"

static int fpu_match(device_t, cfdata_t, void *);
static void fpu_attach(device_t, device_t, void *);
static register_t fpu_identify(void);

CFATTACH_DECL_NEW(fpu, sizeof(struct fpu_softc),
    fpu_match, fpu_attach, NULL, NULL);

struct fpu_softc *the_fpu;

static int
fpu_match(device_t parent, cfdata_t cf, void *aux)
{

	return the_fpu == NULL && fpu_identify() != 0;
}

static void
fpu_attach(device_t parent, device_t self, void *aux)
{
	struct fpu_softc *sc = device_private(self);
	int supported;

	the_fpu = sc;
	printf(": ");
	sc->sc_fputype = fpu_identify();
	supported = 0;
	switch (sc->sc_fputype) {
	case FPSR_SYSID_FPPC:
		printf("FPPC/WE32206");
#ifdef FPU_FPPC
		/* XXX Uncomment when we have minimal support. */
		supported = 1;
		sc->sc_ctxload = fpctx_load_fppc;
		sc->sc_ctxsave = fpctx_save_fppc;
		sc->sc_enable = fpu_enable_fppc;
		sc->sc_disable = fpu_disable_fppc;
#endif
		break;
	case FPSR_SYSID_FPA:
		printf("FPA");
#ifdef FPU_FPA
		/* XXX Uncomment when we have minimal support. */
		supported = 1;
		sc->sc_ctxload = fpctx_load_fpa;
		sc->sc_ctxsave = fpctx_save_fpa;
		sc->sc_enable = fpu_enable_fpa;
		sc->sc_disable = fpu_disable_fpa;
#endif
		break;
	default:
		printf("Unknown type, ID=0x%02x", sc->sc_fputype >> 24);
		break;
	}
	printf("\n");
	if (!supported)
		printf("%s: WARNING: FPU type not supported by kernel\n",
		       device_xname(self));
}

static label_t undef_jmp;

static int
fpu_undef_handler(u_int addr, u_int insn, struct trapframe *tf, int fault_code)
{

	longjmp(&undef_jmp);
}

static register_t
fpu_identify(void)
{
	volatile register_t fpsr;
	void *uh;

	if (setjmp(&undef_jmp) == 0) {
		uh = install_coproc_handler(FPA_COPROC, fpu_undef_handler);
		fpsr = 0;
		__asm volatile ("rfs %0" : "=r" (fpsr));
	}
	remove_coproc_handler(uh);
	return fpsr & FPSR_SYSID_MASK;
}
