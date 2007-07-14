/*	$NetBSD: plcomvar.h,v 1.4 2007/07/14 21:48:19 ad Exp $	*/

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

#include "rnd.h"
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_plcom.h"
#include "opt_kgdb.h"

#if NRND > 0 && defined(RND_COM)
#include <sys/rnd.h>
#endif

#include <sys/callout.h>
#include <sys/timepps.h>
#include <sys/lock.h>

int  plcomcnattach	(bus_space_tag_t, bus_addr_t, int, int, tcflag_t, int);
void plcomcndetach	(void);

#ifdef KGDB
int  plcom_kgdb_attach	(bus_space_tag_t, bus_addr_t, int, int, tcflag_t);
#endif

int  plcom_is_console	(bus_space_tag_t, int, bus_space_handle_t *);

/* Hardware flag masks */
#define	PLCOM_HW_NOIEN		0x01
#define	PLCOM_HW_FIFO		0x02
#define	PLCOM_HW_HAYESP		0x04
#define	PLCOM_HW_FLOW		0x08
#define	PLCOM_HW_DEV_OK		0x20
#define	PLCOM_HW_CONSOLE	0x40
#define	PLCOM_HW_KGDB		0x80
#define	PLCOM_HW_TXFIFO_DISABLE	0x100

/* Buffer size for character buffer */
#define	PLCOM_RING_SIZE		2048

struct plcom_softc {
	struct device sc_dev;
	void *sc_si;
	struct tty *sc_tty;

	struct callout sc_diag_callout;

	bus_addr_t sc_iounit;
	int sc_frequency;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	u_int sc_overflows,
	      sc_floods,
	      sc_errors;

	int sc_hwflags,
	    sc_swflags;
	u_int sc_fifolen;

	u_int sc_r_hiwat,
	      sc_r_lowat;
	u_char *volatile sc_rbget,
	       *volatile sc_rbput;
 	volatile u_int sc_rbavail;
	u_char *sc_rbuf,
	       *sc_ebuf;

 	u_char *sc_tba;
 	u_int sc_tbc,
	      sc_heldtbc;

	volatile u_char sc_rx_flags,
#define	RX_TTY_BLOCKED		0x01
#define	RX_TTY_OVERFLOWED	0x02
#define	RX_IBUF_BLOCKED		0x04
#define	RX_IBUF_OVERFLOWED	0x08
#define	RX_ANY_BLOCK		0x0f
			sc_tx_busy,
			sc_tx_done,
			sc_tx_stopped,
			sc_st_check,
			sc_rx_ready;

	volatile u_char sc_heldchange;
	volatile u_char sc_msr, sc_msr_delta, sc_msr_mask, sc_mcr,
	    sc_mcr_active, sc_lcr, sc_cr, sc_dlbl, sc_dlbh;
	u_char sc_mcr_dtr, sc_mcr_rts, sc_msr_cts, sc_msr_dcd;
	u_int sc_fifo;

	/* Support routine to program mcr lines, if present.  */
	void	(*sc_set_mcr)(void *, int, u_int);
	void	*sc_set_mcr_arg;

	/* power management hooks */
	int (*enable) (struct plcom_softc *);
	void (*disable) (struct plcom_softc *);
	int enabled;

	/* PPS signal on DCD, with or without inkernel clock disciplining */
	u_char	sc_ppsmask;			/* pps signal mask */
	u_char	sc_ppsassert;			/* pps leading edge */
	u_char	sc_ppsclear;			/* pps trailing edge */
	pps_info_t ppsinfo;
	pps_params_t ppsparam;

#if NRND > 0 && defined(RND_COM)
	rndsource_element_t  rnd_source;
#endif
	struct simplelock	sc_lock;
};

int  plcomprobe1	(bus_space_tag_t, bus_space_handle_t);
int  plcomintr		(void *);
void plcom_attach_subr	(struct plcom_softc *);
int  plcom_detach	(struct device *, int);
int  plcom_activate	(struct device *, enum devact);

