/*	$NetBSD: plcomvar.h,v 1.15 2014/02/21 16:08:19 skrll Exp $	*/

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

#ifdef RND_COM
#include <sys/rnd.h>
#endif

#include <sys/callout.h>
#include <sys/timepps.h>

struct plcom_instance;

int  plcomcnattach	(struct plcom_instance *, int, int, tcflag_t, int);
void plcomcndetach	(void);

#ifdef KGDB
int  plcom_kgdb_attach	(struct plcom_instance *, int, int, tcflag_t, int);
#endif

int  plcom_is_console	(bus_space_tag_t, bus_addr_t, bus_space_handle_t *);

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

struct plcom_instance {
	u_int			pi_type;
#define	PLCOM_TYPE_PL010 0
#define	PLCOM_TYPE_PL011 1

	uint32_t		pi_flags;	/* flags for this PLCOM */
#define	PLC_FLAG_USE_DMA		0x0001
#define	PLC_FLAG_32BIT_ACCESS		0x0002

	void 			*pi_cookie;

	bus_space_tag_t		pi_iot;
	bus_space_handle_t	pi_ioh;
	bus_addr_t		pi_iobase;
	bus_addr_t		pi_size;
	struct plcom_registers	*pi_regs;
};

struct plcomcons_info {
	int	rate;
	int	frequency;
	int	type;

	tcflag_t	cflag;
};
    
struct plcom_softc {
	device_t sc_dev;

	struct tty *sc_tty;
	void *sc_si;

	struct callout sc_diag_callout;

 	int sc_frequency;

	struct plcom_instance sc_pi;

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
	volatile u_int sc_cr, sc_ratel, sc_rateh, sc_imsc;
	volatile u_int sc_msr, sc_msr_delta, sc_msr_mask;
	volatile u_char sc_mcr, sc_mcr_active, sc_lcr;
	u_char sc_mcr_dtr, sc_mcr_rts, sc_msr_cts, sc_msr_dcd;
	u_int sc_fifo;

	/* Support routine to program mcr lines for PL010, if present.  */
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

#ifdef RND_COM
	krndsource_t  rnd_source;
#endif
	kmutex_t		sc_lock;
};

#if 0
int  plcomprobe1	(bus_space_tag_t, bus_space_handle_t);
#endif
int  plcomintr		(void *);
void plcom_attach_subr	(struct plcom_softc *);
int  plcom_detach	(device_t, int);
int  plcom_activate	(device_t, enum devact);

