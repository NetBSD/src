/* $NetBSD: si.h,v 1.1 2026/01/09 22:54:30 jmcneill Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _WII_DEV_SI_H_
#define _WII_DEV_SI_H_

#include <dev/hid/hidev.h>
#include <sys/workqueue.h>

#define SI_NUM_CHAN		4

#define SICOUTBUF(n)		((n) * 0xc + 0x00)
#define SICINBUFH(n)		((n) * 0xc + 0x04)
#define SICINBUFL(n)		((n) * 0xc + 0x08)
#define SIPOLL			0x30
#define  SIPOLL_X		__BITS(25, 16)
#define  SIPOLL_Y		__BITS(15, 8)
#define  SIPOLL_EN(n)		(__BIT(4 + (n)))
#define SICOMCSR		0x34
#define  SICOMCSR_TCINT		__BIT(31)
#define  SICOMCSR_TCINTMSK	__BIT(30)
#define  SICOMCSR_COMERR	__BIT(29)
#define  SICOMCSR_RDSTINT	__BIT(28)
#define  SICOMCSR_RDSTINTMSK	__BIT(27)
#define  SICOMCSR_CH_EN    	__BIT(24)
#define  SICOMCSR_OUTLNGTH	__BITS(22, 16)
#define  SICOMCSR_INLNGTH	__BITS(14, 8)
#define  SICOMCSR_CMD_EN   	__BIT(7)
#define  SICOMCSR_CHANNEL	__BITS(2, 1)
#define  SICOMCSR_TSTART	__BIT(0)
#define SISR			0x38
#define  SISR_SICNOUTBUF	__BIT(31)
#define  SISR_OFF(n)		((3 - (n)) * 8)
#define  SISR_WR(n)		__BIT(SISR_OFF(n) + 7)
#define  SISR_RDST(n)		__BIT(SISR_OFF(n) + 5)
#define  SISR_WRST(n)		__BIT(SISR_OFF(n) + 4)
#define  SISR_NOREP(n)		__BIT(SISR_OFF(n) + 3)
#define  SISR_COLL(n)		__BIT(SISR_OFF(n) + 2)
#define  SISR_OVRUN(n)		__BIT(SISR_OFF(n) + 1)
#define  SISR_UNRUN(n)		__BIT(SISR_OFF(n) + 0)
#define  SISR_ERROR_MASK(n)	(SISR_NOREP(n) | SISR_COLL(n) | \
				 SISR_OVRUN(n) | SISR_UNRUN(n))
#define  SISR_ERROR_ACK_ALL	(SISR_ERROR_MASK(0) | SISR_ERROR_MASK(1) | \
				 SISR_ERROR_MASK(2) | SISR_ERROR_MASK(3))
#define SIEXILK			0x3c
#define SIIOBUF			0x80
#define SIIOBUF_SIZE		128

#define GCPAD_REPORT_SIZE	9
#define GCPAD_ERRSTAT(_buf)	ISSET((_buf)[0], __BIT(31))
#define GCPAD_ERRLATCH(_buf)	ISSET((_buf)[0], __BIT(30))
#define GCPAD_ERR(_buf)		(GCPAD_ERRSTAT(_buf)) || (GCPAD_ERRLATCH(_buf))
#define GCPAD_START(_buf)	ISSET((_buf)[0], 0x10)
#define GCPAD_Y(_buf)		ISSET((_buf)[0], 0x08)
#define GCPAD_X(_buf)		ISSET((_buf)[0], 0x04)
#define GCPAD_B(_buf)		ISSET((_buf)[0], 0x02)
#define GCPAD_A(_buf)		ISSET((_buf)[0], 0x01)
#define GCPAD_LCLICK(_buf)	ISSET((_buf)[1], 0x40)
#define GCPAD_RCLICK(_buf)	ISSET((_buf)[1], 0x20)
#define GCPAD_Z(_buf)		ISSET((_buf)[1], 0x10)
#define GCPAD_UP(_buf)		ISSET((_buf)[1], 0x08)
#define GCPAD_DOWN(_buf)	ISSET((_buf)[1], 0x04)
#define GCPAD_RIGHT(_buf)	ISSET((_buf)[1], 0x02)
#define GCPAD_LEFT(_buf)	ISSET((_buf)[1], 0x01)

#define TXN_MAX 	12
#define TXN_READY 	-1
#define TXN_DEQUEUED	0
#define TXN_USEC	250000

struct si_softc;

struct si_channel {
	struct si_softc		*ch_sc;
	device_t		ch_dev;
	device_t		ch_gcport_dev;
	device_t		ch_uhid_dev;
	unsigned		ch_index;
	struct hidev_tag	ch_hidev;
	kmutex_t		ch_lock;
	kcondvar_t		ch_cv;
	uint8_t			ch_state;
#define SI_STATE_OPEN		__BIT(0)
#define SI_STATE_STOPPED	__BIT(1)
	void			(*ch_intr)(void *, void *, u_int);
	void			*ch_intrarg;
	uint8_t			ch_buf[GCPAD_REPORT_SIZE];
	void			*ch_desc;
	int			ch_descsize;
	void			*ch_si;
	struct workqueue	*ch_wqp;
	struct work		ch_work;
};

struct si_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct si_channel	sc_chan[SI_NUM_CHAN];
};

struct si_attach_args {
	struct hidev_tag	*saa_hidev;
	int			saa_index;
};

struct gcport_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct si_channel	*ch;
};

struct si_packet {
	unsigned	chan;		/* which controller port */
	uint32_t	status;		/* the sisr result for this channel */
	uint32_t	insize;		/* number of bytes for in buffer */
	uint32_t	outsize;	/* number of bytes for out buffer */
	uint32_t	*in;		/* buffer to store response */
	uint32_t	*out;		/* buffer to send out to ext device */
};

struct sicomcsr_txn {
	unsigned			status;		/* txn current state */
	uint32_t			comcsr;		/* write to sicomcsr */
	kcondvar_t			cv;		/* signal completion */
	kmutex_t			lock;		/* cv interlock */
	struct si_packet		*pk;		/* siiobuf data */
	TAILQ_ENTRY(sicomcsr_txn)	txn_q;
};

void 	txn_init(struct sicomcsr_txn *);
int 	txn_enqueue(struct sicomcsr_txn *);
void	txn_dequeue(struct sicomcsr_txn *);
int 	txn_await(struct sicomcsr_txn *);
void 	txn_destroy(struct sicomcsr_txn *);
#endif /* _WII_DEV_SI_H_ */
