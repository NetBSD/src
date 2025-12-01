/*	$NetBSD: sc16is7xx_tty.c,v 1.4 2025/12/01 14:56:03 brad Exp $	*/

/*
 * Copyright (c) 2025 Brad Spencer <brad@anduin.eldar.org>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sc16is7xx_tty.c,v 1.4 2025/12/01 14:56:03 brad Exp $");

/* TTY specific common driver to the NXP SC16IS7xx UART bridge */

#include <sys/kthread.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/tty.h>
#include <sys/systm.h>

#include <sys/bus.h>

#include <dev/ic/sc16is7xxreg.h>
#include <dev/ic/sc16is7xxvar.h>
#include <dev/ic/sc16is7xx_ttyvar.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

static int sc16is7xx_tty_match(device_t, cfdata_t, void *);
static void sc16is7xx_tty_attach(device_t, device_t, void *);
static int sc16is7xx_tty_detach(device_t, int);

CFATTACH_DECL_NEW(sc16is7xx_tty, sizeof(struct sc16is7xx_tty_softc),
    sc16is7xx_tty_match, sc16is7xx_tty_attach, sc16is7xx_tty_detach, NULL);

#define	COM_TO_SC16IS7XX(sc)	\
	container_of((sc), struct sc16is7xx_tty_softc, sc_com)

static int
sc16is7xx_tty_enable(struct com_softc *com_sc)
{
	struct sc16is7xx_tty_softc *sc = COM_TO_SC16IS7XX(com_sc);
	struct sc16is7xx_sc *psc = sc->psc;
	int error = 0;
	
	mutex_enter(&psc->sc_thread_mutex);
	
	if (psc->sc_thread_state != SC16IS7XX_THREAD_GPIO &&
	    psc->sc_thread_state != SC16IS7XX_THREAD_RUNNING &&
	    psc->sc_thread_state != SC16IS7XX_THREAD_EXIT &&
	    psc->sc_de_count == 0) {
		if (psc->sc_thread_state == SC16IS7XX_THREAD_STOPPED)
			error = kthread_create(PRI_SOFTSERIAL, KTHREAD_MUSTJOIN | KTHREAD_MPSAFE, NULL,
			    sc16is7xx_thread, psc, &psc->sc_thread, "%s", device_xname(psc->sc_dev));
		if (error) {
			aprint_error_dev(psc->sc_dev,
			    "Could not create kernel thread for polling: %d\n",
			    error);
		} else {
			psc->sc_thread_state = SC16IS7XX_THREAD_RUNNING;
			cv_signal(&psc->sc_threadvar);
		}
	}

	psc->sc_de_count++;

	KASSERTMSG(psc->sc_de_count >= 0 && psc->sc_de_count <= 2,
	    "Disable / Enable count out of bounds: %d\n", psc->sc_de_count);

	com_sc->sc_regs.cr_has_errored = false;

	mutex_exit(&psc->sc_thread_mutex);

	return 0;
}

static void
sc16is7xx_tty_disable(struct com_softc *com_sc)
{
	struct sc16is7xx_tty_softc *sc = COM_TO_SC16IS7XX(com_sc);
	struct sc16is7xx_sc *psc = sc->psc;
	
	mutex_enter(&psc->sc_thread_mutex);
	
	psc->sc_de_count--;

	KASSERTMSG(psc->sc_de_count >= 0 && psc->sc_de_count <= 2,
	    "Disable / Enable count out of bounds: %d\n", psc->sc_de_count);

	if (psc->sc_thread_state != SC16IS7XX_THREAD_GPIO &&
	    psc->sc_thread_state != SC16IS7XX_THREAD_PAUSED &&
	    psc->sc_thread_state != SC16IS7XX_THREAD_EXIT &&
	    psc->sc_de_count == 0) {
		psc->sc_thread_state = SC16IS7XX_THREAD_PAUSED;
		cv_signal(&psc->sc_threadvar);
	}

	mutex_exit(&psc->sc_thread_mutex);
	
}

static int
sc16is7xx_tty_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

void
sc16is7xx_tty_attach(device_t parent, device_t self, void *aux)
{
	struct sc16is7xx_tty_attach_args *caa = aux;
	struct sc16is7xx_tty_softc *sc = device_private(self);
	struct sc16is7xx_sc *psc = device_private(parent);

	sc->sc_com.sc_dev = self;
	sc->psc = psc;

	device_setprop_bool(self, "is-console", false);
	device_setprop_bool(self, "force-console", false);

	/* Set and then override the callouts to read and write the registers. */
	com_init_regs(&sc->sc_com.sc_regs, 0, 0, 0);
	sc->sc_com.sc_regs.cr_read = psc->sc_com_funcs->com_read_1;
	sc->sc_com.sc_regs.cr_write = psc->sc_com_funcs->com_write_1;
	sc->sc_com.sc_regs.cr_write_multi = psc->sc_com_funcs->com_write_multi_1;
	sc->sc_com.sc_regs.cr_channel = caa->aa_channel;

	sc->sc_com.enable = &sc16is7xx_tty_enable;
	sc->sc_com.disable = &sc16is7xx_tty_disable;

	psc->sc_funcs->copy_handles(psc, &sc->sc_com.sc_regs);

	/* We will get a 64 byte FIFO and hardware flow control if we use this */
	sc->sc_com.sc_type = COM_TYPE_SC16IS7XX;

	/* The master frequency is pushed down from the bus layer as both
	 * channels use the same clock.  However it needs to be set here as
	 * com(4) wants it. */
	sc->sc_com.sc_frequency = psc->sc_frequency;

	/* This will always be 0, as the polling will occur at the bus layer,
	 * if needed. */
	sc->sc_com.sc_poll_ticks = 0;

	/* Tell com(4) to run in a soft context whenever possible */
	SET(sc->sc_com.sc_hwflags, COM_HW_SOFTIRQ);

	com_attach_subr(&sc->sc_com);

	return;
}

int
sc16is7xx_tty_detach(device_t self, int flags)
{
	int error = 0;

	error = com_detach(self, flags);

	return error;
}
