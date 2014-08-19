/*	$NetBSD: ipifuncs.c,v 1.2.6.1 2014/08/20 00:03:04 tls Exp $	*/
/*	$OpenBSD: ipi.c,v 1.4 2011/01/14 13:20:06 jsing Exp $	*/

/*
 * Copyright (c) 2010 Joel Sing <jsing@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/device.h>
#include <sys/atomic.h>
#include <sys/xcall.h>
#include <sys/ipi.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#if 0
#include <machine/fpu.h>
#endif
#include <machine/iomod.h>
#include <machine/intr.h>
#include <machine/mutex.h>
#include <machine/reg.h>
#include <machine/int_fmtio.h>

#include <hppa/hppa/cpuvar.h>

static void hppa_ipi_nop(void);
static void hppa_ipi_halt(void);

void (*ipifunc[HPPA_NIPI])(void) =
{
	hppa_ipi_nop,
	hppa_ipi_halt,
	xc_ipi_handler,
	ipi_cpu_handler,
};

const char *ipinames[HPPA_NIPI] = {
	"nop ipi",
	"halt ipi",
	"xcall ipi"
	"generic ipi"
};

void
hppa_ipi_init(struct cpu_info *ci)
{
	struct cpu_softc *sc = ci->ci_softc;
	int i;

	evcnt_attach_dynamic(&sc->sc_evcnt_ipi, EVCNT_TYPE_INTR,
	    NULL, device_xname(sc->sc_dev), "ipi");

	for (i = 0; i < HPPA_NIPI; i++) {
		evcnt_attach_dynamic(&sc->sc_evcnt_which_ipi[i],
		    EVCNT_TYPE_INTR, NULL, device_xname(sc->sc_dev),
		    ipinames[i]);
	}
}

int
hppa_ipi_intr(void *arg)
{
	struct cpu_info *ci = curcpu();
	struct cpu_softc *sc = ci->ci_softc;
	u_long ipi_pending;
	int bit = 0;

	/* Handle an IPI. */
	ipi_pending = atomic_swap_ulong(&ci->ci_ipi, 0);

	KASSERT(ipi_pending);

	sc->sc_evcnt_ipi.ev_count++;

	while (ipi_pending) {
		if (ipi_pending & (1L << bit)) {
			sc->sc_evcnt_which_ipi[bit].ev_count++;
			(*ipifunc[bit])();
		}
		ipi_pending &= ~(1L << bit);
		bit++;
	}

	return 1;
}

int
hppa_ipi_send(struct cpu_info *ci, u_long ipi)
{
	struct iomod *cpu;
	KASSERT(ci->ci_flags & CPUF_RUNNING);

	atomic_or_ulong(&ci->ci_ipi, (1L << ipi));

	/* Send an IPI to the specified CPU by triggering EIR{1} (irq 30). */
	cpu = (struct iomod *)(ci->ci_hpa);
	cpu->io_eir = 1;
	membar_sync();

	return 0;
}

int
hppa_ipi_broadcast(u_long ipi)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	int count = 0;

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci != curcpu() && (ci->ci_flags & CPUF_RUNNING))
			if (hppa_ipi_send(ci, ipi))
				count++;
	}
	
	return count;	
}

static void
hppa_ipi_nop(void)
{
}

static void
hppa_ipi_halt(void)
{
	struct cpu_info *ci = curcpu();

	/* Turn off interrupts and halt CPU. */
// 	hppa_intr_disable();
	ci->ci_flags &= ~CPUF_RUNNING;

	for (;;)
		;
}

void
xc_send_ipi(struct cpu_info *ci)
{
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);

	if (ci) {
		/* Unicast: remote CPU. */
		hppa_ipi_send(ci, HPPA_IPI_XCALL);
	} else {
		/* Broadcast: all, but local CPU (caller will handle it). */
		hppa_ipi_broadcast(HPPA_IPI_XCALL);
	}
}

void
cpu_ipi(struct cpu_info *ci)
{
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);

	if (ci) {
		/* Unicast: remote CPU. */
		hppa_ipi_send(ci, HPPA_IPI_GENERIC);
	} else {
		/* Broadcast: all, but local CPU (caller will handle it). */
		hppa_ipi_broadcast(HPPA_IPI_GENERIC);
	}
}
