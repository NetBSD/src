/*	$NetBSD: comvar.h,v 1.98 2022/10/08 07:27:03 riastradh Exp $	*/

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

#ifndef	_SYS_DEV_IC_COMVAR_H_
#define	_SYS_DEV_IC_COMVAR_H_

#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_com.h"
#include "opt_kgdb.h"

#ifdef RND_COM
#include <sys/rndsource.h>
#endif

#include <sys/callout.h>
#include <sys/timepps.h>
#include <sys/mutex.h>
#include <sys/device.h>

#include <dev/ic/comreg.h>	/* for COM_NPORTS */

struct com_regs;

int comcnattach(bus_space_tag_t, bus_addr_t, int, int, int, tcflag_t);
int comcnattach1(struct com_regs *, int, int, int, tcflag_t);

#ifdef KGDB
int com_kgdb_attach(bus_space_tag_t, bus_addr_t, int, int, int, tcflag_t);
int com_kgdb_attach1(struct com_regs *, int, int, int, tcflag_t);
#endif

int com_is_console(bus_space_tag_t, bus_addr_t, bus_space_handle_t *);

/* Hardware flag masks */
#define	COM_HW_NOIEN	0x01
#define	COM_HW_FIFO	0x02
#define	COM_HW_BROKEN_ETXRDY	0x04
#define	COM_HW_FLOW	0x08
#define	COM_HW_DEV_OK	0x20
#define	COM_HW_CONSOLE	0x40
#define	COM_HW_KGDB	0x80
#define	COM_HW_TXFIFO_DISABLE	0x100
#define	COM_HW_NO_TXPRELOAD	0x200
#define	COM_HW_AFE	0x400

/* Buffer size for character buffer */
#ifndef COM_RING_SIZE
#define	COM_RING_SIZE	2048
#endif

#define	COM_REG_RXDATA		0
#define	COM_REG_TXDATA		1
#define	COM_REG_DLBL		2
#define	COM_REG_DLBH		3
#define	COM_REG_IER		4
#define	COM_REG_IIR		5
#define	COM_REG_FIFO		6
#define	COM_REG_TCR		7
#define	COM_REG_EFR		8
#define	COM_REG_TLR		9
#define	COM_REG_LCR		10
#define	COM_REG_MCR		11
#define	COM_REG_LSR		12
#define	COM_REG_MSR		13
#define	COM_REG_MDR1		14		/* TI OMAP */
#define	COM_REG_USR		15		/* 16750/DW APB */
#define	COM_REG_TFL		16		/* DW APB */
#define	COM_REG_RFL		17		/* DW APB */
#define	COM_REG_HALT		18		/* DW APB */

#define	COM_REGMAP_NENTRIES	19

struct com_regs {
	bus_space_tag_t		cr_iot;
	bus_space_handle_t	cr_ioh;
	bus_addr_t		cr_iobase;
	bus_size_t		cr_nports;
	bus_size_t		cr_map[COM_REGMAP_NENTRIES];
	uint8_t			(*cr_read)(struct com_regs *, u_int);
	void			(*cr_write)(struct com_regs *, u_int, uint8_t);
	void			(*cr_write_multi)(struct com_regs *, u_int,
						  const uint8_t *,
						  bus_size_t);
	
};

void	com_init_regs(struct com_regs *, bus_space_tag_t, bus_space_handle_t,
		      bus_addr_t);
void	com_init_regs_stride(struct com_regs *, bus_space_tag_t,
			     bus_space_handle_t, bus_addr_t, u_int);
void	com_init_regs_stride_width(struct com_regs *, bus_space_tag_t,
				   bus_space_handle_t, bus_addr_t, u_int, u_int);

struct comcons_info {
	struct com_regs regs;
	int rate;
	int frequency;
	int type;
	tcflag_t cflag;
};

struct com_softc {
	device_t sc_dev;
	void *sc_si;
	struct tty *sc_tty;

	callout_t sc_diag_callout;
	callout_t sc_poll_callout;
	struct timeval sc_hup_pending;

	int sc_frequency;

	struct com_regs sc_regs;
	bus_space_handle_t sc_hayespioh;


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
	    sc_mcr_active, sc_lcr, sc_ier, sc_fifo, sc_dlbl, sc_dlbh, sc_efr;
	u_char sc_mcr_dtr, sc_mcr_rts, sc_msr_cts, sc_msr_dcd;

	u_char sc_prescaler;		/* for COM_TYPE_HAYESP */

	/*
	 * There are a great many almost-ns16550-compatible UARTs out
	 * there, which have minor differences.  The type field here
	 * lets us distinguish between them.
	 */
	int sc_type;
#define	COM_TYPE_NORMAL		0	/* normal 16x50 */
#define	COM_TYPE_HAYESP		1	/* Hayes ESP modem */
#define	COM_TYPE_PXA2x0		2	/* Intel PXA2x0 processor built-in */
#define	COM_TYPE_AU1x00		3	/* AMD/Alchemy Au1x000 proc. built-in */
#define	COM_TYPE_OMAP		4	/* TI OMAP processor built-in */
#define	COM_TYPE_16550_NOERS	5	/* like a 16550, no ERS */
#define	COM_TYPE_INGENIC	6	/* JZ4780 built-in */
#define	COM_TYPE_TEGRA		7	/* NVIDIA Tegra built-in */
#define	COM_TYPE_BCMAUXUART	8	/* BCM2835 AUX UART */
#define	COM_TYPE_16650		9
#define	COM_TYPE_16750		10
#define	COM_TYPE_DW_APB		11	/* DesignWare APB UART */

	int sc_poll_ticks;

	/* power management hooks */
	int (*enable)(struct com_softc *);
	void (*disable)(struct com_softc *);
	int enabled;

	struct pps_state sc_pps_state;	/* pps state */

#ifdef RND_COM
	krndsource_t  rnd_source;
#endif
	kmutex_t		sc_lock;
};

int comprobe1(bus_space_tag_t, bus_space_handle_t);
int comintr(void *);
void com_attach_subr(struct com_softc *);
int com_probe_subr(struct com_regs *);
int com_detach(device_t, int);
bool com_resume(device_t, const pmf_qual_t *);
bool com_cleanup(device_t, int);
bool com_suspend(device_t, const pmf_qual_t *);

#ifndef IPL_SERIAL
#define	IPL_SERIAL	IPL_TTY
#define	splserial()	spltty()
#endif

#endif	/* _SYS_DEV_IC_COMVAR_H_ */
