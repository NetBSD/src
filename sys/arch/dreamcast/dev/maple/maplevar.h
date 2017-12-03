/*	$NetBSD: maplevar.h,v 1.13.18.2 2017/12/03 11:36:01 jdolecek Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by ITOH Yasufumi.
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

/*-
 * Copyright (c) 2001 Marcus Comstedt
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
 *	This product includes software developed by Marcus Comstedt.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/queue.h>

#define MAPLE_PORTS 4
#define MAPLE_SUBUNITS 6

#define MAPLE_NFUNC	32

struct maple_func {
	int		f_funcno;
	struct maple_unit *f_unit;
	device_t f_dev;

	/* callback */
	void		(*f_callback)(void *, struct maple_response *,
			    int /*len*/, int /*flags*/);
	void 		*f_arg;

	uint32_t	f_work;		/* for periodic GETCOND and ping */

	/* periodic command request */
	enum maple_periodic_stat {
		MAPLE_PERIODIC_NONE,
		MAPLE_PERIODIC_INQ,
		MAPLE_PERIODIC_DEFERED
	} f_periodic_stat;
	TAILQ_ENTRY(maple_func)	f_periodicq;

	/* command request */
	int		f_command;
	int		f_datalen;
	const void	*f_dataaddr;
	enum maple_command_stat {
		MAPLE_CMDSTAT_NONE,		/* not in queue */
		MAPLE_CMDSTAT_ASYNC,		/* process immediately */
		MAPLE_CMDSTAT_PERIODIC_DEFERED,	/* periodic but process imdtly*/
		MAPLE_CMDSTAT_ASYNC_PERIODICQ,	/* async but on periodic queue*/
		MAPLE_CMDSTAT_PERIODIC		/* process on periodic timing */
	} f_cmdstat;
	TAILQ_ENTRY(maple_func)	f_cmdq;
};

/* work-around problem with 3rd party memory cards */
#define MAPLE_MEMCARD_PING_HACK

struct maple_unit {
	int		port, subunit;
	struct maple_func u_func[MAPLE_NFUNC];
	uint32_t	getcond_func_set;
	int		u_ping_func;	/* function used for ping */
	uint32_t	u_noping;	/* stop ping (bitmap of function) */
#ifdef MAPLE_MEMCARD_PING_HACK
	enum maple_ping_stat {
		MAPLE_PING_NORMAL,	/* ping with GETCOND */
		MAPLE_PING_MEMCARD,	/* memory card, possibly 3rd party */
		MAPLE_PING_MINFO	/* poorly implemented 3rd party card */
	} u_ping_stat;
#endif
	struct maple_devinfo devinfo;

	/* DMA status / function */
	enum maple_dma_stat {
		MAPLE_DMA_IDLE,		/* not in queue */
		MAPLE_DMA_RETRY,	/* retrying last command (sc_retryq) */
		MAPLE_DMA_PERIODIC,	/* periodic GETCOND */
		MAPLE_DMA_ACMD,		/* asynchronous command */
		MAPLE_DMA_PCMD,		/* command on periodic timing */
		MAPLE_DMA_PROBE,	/* checking for insertion */
		MAPLE_DMA_PING		/* checking for removal */
	} u_dma_stat;
	int	u_dma_func;

	SIMPLEQ_ENTRY(maple_unit)	u_dmaq;

	/* start of each receive buffer */
	uint32_t	*u_rxbuf;
  	uint32_t	u_rxbuf_phys;

	/* for restarting command */
	int		u_command;
	int		u_datalen;
	const void	*u_dataaddr;
	enum maple_dma_stat u_saved_dma_stat;
	int		u_retrycnt;
#define MAPLE_RETRY_MAX	100	/* ~2s */
	/*
	 * The 2s retry is rather too long, but required to avoid
	 * unwanted detach/attach.
	 * If a Visual Memory (without cells) is inserted to a controller,
	 * the controller (including the base device and the other unit
	 * in the slot) stops responding for near 1 second.  If two VM are
	 * inserted in succession, the period becomes near 2s.
	 */

	/* queue for probe/ping */
	enum maple_queue_stat {
		MAPLE_QUEUE_NONE,	/* not in queue */
		MAPLE_QUEUE_PROBE,	/* checking for insertion */
		MAPLE_QUEUE_PING	/* checking for removal */
	} u_queuestat;
	TAILQ_ENTRY(maple_unit)	u_q;
	int			u_proberetry;	/* retry count (subunit != 0) */
#define MAPLE_PROBERETRY_MAX	5
};

struct maple_softc {
	device_t	sc_dev;

	callout_t	maple_callout_ch;
	lwp_t		*event_thread;

	int8_t		sc_port_unit_map[MAPLE_PORTS];
	int		sc_port_units[MAPLE_PORTS];
	int		sc_port_units_open[MAPLE_PORTS];

	struct maple_unit sc_unit[MAPLE_PORTS][MAPLE_SUBUNITS];

	uint32_t *sc_txbuf;	/* start of allocated transmit buffer */
	uint32_t *sc_txpos;	/* current write position in tx buffer */
	uint32_t *sc_txlink;	/* start of last written frame */

	uint32_t sc_txbuf_phys;	/* 29-bit physical address */

	void	*sc_intrhand;

	kmutex_t sc_dma_lock;
	kcondvar_t sc_dma_cv;

	int	sc_event;	/* periodic event is active */
	kmutex_t sc_event_lock;
	kcondvar_t sc_event_cv;

	SIMPLEQ_HEAD(maple_dmaq_head, maple_unit) sc_dmaq, sc_retryq;
	TAILQ_HEAD(maple_unitq_head, maple_unit) sc_probeq, sc_pingq;
	TAILQ_HEAD(maple_fnq_head, maple_func) sc_periodicq, sc_periodicdeferq;
	TAILQ_HEAD(maple_cmdq_head, maple_func) sc_acmdq, sc_pcmdq;
};
