/*	$NetBSD: iop.c,v 1.10.26.1 2007/03/24 14:54:48 yamt Exp $	*/

/*
 * Copyright (c) 2000 Allen Briggs.
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
 *	This code handles VIA, RBV, and OSS functionality.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iop.c,v 1.10.26.1 2007/03/24 14:54:48 yamt Exp $");

#include "opt_mac68k.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/queue.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/frame.h>

#include <machine/iopreg.h>
#include <machine/viareg.h>

static IOP	mac68k_iops[2];

static void	iopism_hand(void *);
static void	load_msg_to_iop(IOPHW *, struct iop_msg *);
static void	iop_message_sent(IOP *, int);
static void	receive_iop_message(IOP *, int);
static void	default_listener(IOP *, struct iop_msg *);

static inline int iop_alive(IOPHW *);
static inline int iop_read1(IOPHW *, u_long);
static inline void iop_write1(IOPHW *, u_long, u_char);
static inline void _iop_upload(IOPHW *, u_char *, u_long, u_long);
static inline void _iop_download(IOPHW *, u_char *, u_long, u_long);

static inline int
iop_read1(IOPHW *ioph, u_long iopbase)
{
	IOP_LOADADDR(ioph, iopbase);
	return ioph->data;
}

static inline void
iop_write1(IOPHW *ioph, u_long iopbase, u_char data)
{
	IOP_LOADADDR(ioph, iopbase);
	ioph->data = data;
}

static inline int
iop_alive(IOPHW *ioph)
{
	int alive;

	alive = iop_read1(ioph, IOP_ADDR_ALIVE);
	iop_write1(ioph, IOP_ADDR_ALIVE, 0);
	return alive;
}

static void
default_listener(IOP *iop, struct iop_msg *msg)
{
	printf("unsolicited message on channel %d.\n", msg->channel);
}

void
iop_init(int fullinit)
{
	IOPHW *ioph;
	IOP *iop;
	int i, ii;

	switch (current_mac_model->machineid) {
	default:
		return;
	case MACH_MACQ900:
	case MACH_MACQ950:
		mac68k_iops[SCC_IOP].iop = (IOPHW *)
						 ((u_char *)IOBase +  0xc000);
		mac68k_iops[ISM_IOP].iop = (IOPHW *)
						 ((u_char *)IOBase + 0x1e000);
		break;
	case MACH_MACIIFX:
		mac68k_iops[SCC_IOP].iop = (IOPHW *)
						 ((u_char *)IOBase +  0x4000);
		mac68k_iops[ISM_IOP].iop = (IOPHW *)
						 ((u_char *)IOBase + 0x12000);
		break;
	}       

	if (!fullinit) {
		ioph = mac68k_iops[SCC_IOP].iop;
		ioph->control_status = 0;		/* Reset */
		ioph->control_status = IOP_BYPASS;	/* Set to bypass */

		ioph = mac68k_iops[ISM_IOP].iop;
		ioph->control_status = 0;		/* Reset */

		return;
	}

	for (ii = 0 ; ii < 2 ; ii++) {
		iop = &mac68k_iops[ii];
		ioph = iop->iop;
		for (i = 0; i < IOP_MAXCHAN; i++) {
			SIMPLEQ_INIT(&iop->sendq[i]);
			SIMPLEQ_INIT(&iop->recvq[i]);
			iop->listeners[i] = default_listener;
			iop->listener_data[i] = NULL;
		}
/*		IOP_LOADADDR(ioph, 0x200);
		for (i = 0x200; i > 0; i--) {
			ioph->data = 0;
		}*/
	}

	switch (current_mac_model->machineid) {
	default:
		return;
	case MACH_MACQ900:
	case MACH_MACQ950:
#ifdef notyet_maybe_not_ever
		iop = &mac68k_iops[SCC_IOP];
		intr_establish(iopscc_hand, iop, 4);
#endif
		iop = &mac68k_iops[ISM_IOP];
		via2_register_irq(0, iopism_hand, iop);
		via_reg(VIA2, vIER) = 0x81;
		via_reg(VIA2, vIFR) = 0x01;
		break;
	case MACH_MACIIFX:
		/* oss_register_irq(2, iopism_hand, &ioph); */
		break;
	}

	iop = &mac68k_iops[SCC_IOP];
	ioph = iop->iop;
	printf("SCC IOP base: 0x%x\n", (unsigned) ioph);
	pool_init(&iop->pool, sizeof(struct iop_msg), 0, 0, 0, "mac68k_iop1",
	    NULL, IPL_NONE);
	ioph->control_status = IOP_BYPASS;

	iop = &mac68k_iops[ISM_IOP];
	ioph = iop->iop;
	printf("ISM IOP base: 0x%x, alive %x\n", (unsigned) ioph, 
	(unsigned) iop_alive(ioph));
	pool_init(&iop->pool, sizeof(struct iop_msg), 0, 0, 0, "mac68k_iop2",
	    NULL, IPL_NONE);
	iop_write1(ioph, IOP_ADDR_ALIVE, 0);

/*
 * XXX  The problem here seems to be that the IOP wants to go back into
 *	BYPASS mode.  The state should be 0x86 after we're done with it
 *	here.  It switches to 0x7 almost immediately.
 *	This means one of a couple of things to me--
 *		1. We're doing something wrong
 *		2. MacOS is really shutting down the IOP
 *	Most likely, it's the first.
 */
	printf("OLD cs0: 0x%x\n", (unsigned) ioph->control_status);

	ioph->control_status = IOP_CS_RUN | IOP_CS_AUTOINC;
{unsigned cs, c2;
	cs = (unsigned) ioph->control_status;
	printf("OLD cs1: 0x%x\n", cs);
	cs = 0;
	do { c2 = iop_read1(ioph, IOP_ADDR_ALIVE); cs++; } while (c2 != 0xff);
	printf("OLD cs2: 0x%x (i = %d)\n", (unsigned) ioph->control_status, cs);
}
}

static inline void
_iop_upload(IOPHW *ioph, u_char *mem, u_long nb, u_long iopbase)
{
	IOP_LOADADDR(ioph, iopbase);
	while (nb--) {
		ioph->data = *mem++;
	}
}

void
iop_upload(int iopn, u_char *mem, u_long nb, u_long iopbase)
{
	IOPHW *ioph;

	if (iopn & ~1) return;
	ioph = mac68k_iops[iopn].iop;
	if (!ioph) return;

	_iop_upload(ioph, mem, nb, iopbase);
}

static inline void
_iop_download(IOPHW *ioph, u_char *mem, u_long nb, u_long iopbase)
{
	IOP_LOADADDR(ioph, iopbase);
	while (nb--) {
		*mem++ = ioph->data;
	}
}

void
iop_download(int iopn, u_char *mem, u_long nb, u_long iopbase)
{
	IOPHW	*ioph;

	if (iopn & ~1) return;
	ioph = mac68k_iops[iopn].iop;
	if (!ioph) return;

	_iop_download(ioph, mem, nb, iopbase);
}

static void
iopism_hand(void *arg)
{
	IOP *iop;
	IOPHW *ioph;
	u_char cs;
	u_char m, s;
	int i;

	iop = (IOP *) arg;
	ioph = iop->iop;
	cs = ioph->control_status;

printf("iopism_hand.\n");

#if DIAGNOSTIC
	if ((cs & IOP_INTERRUPT) == 0) {
		printf("IOP_ISM interrupt--no interrupt!? (cs 0x%x)\n",
			(u_int) cs);
	}
#endif

	/*
	 * Scan send queues for complete messages.
	 */
	if (cs & IOP_CS_INT0) {
		ioph->control_status |= IOP_CS_INT0;
		m = iop_read1(ioph, IOP_ADDR_MAX_SEND_CHAN);
		for (i = 0; i < m; i++) {
			s = iop_read1(ioph, IOP_ADDR_SEND_STATE + i);
			if (s == IOP_MSG_COMPLETE) {
				iop_message_sent(iop, i);
			}
		}
	}

	/*
	 * Scan receive queue for new messages.
	 */
	if (cs & IOP_CS_INT1) {
		ioph->control_status |= IOP_CS_INT1;
		m = iop_read1(ioph, IOP_ADDR_MAX_RECV_CHAN);
		for (i = 0; i < m; i++) {
			s = iop_read1(ioph, IOP_ADDR_RECV_STATE + i);
			if (s == IOP_MSG_NEW) {
				receive_iop_message(iop, i);
			}
		}
	}
}

static void
load_msg_to_iop(IOPHW *ioph, struct iop_msg *msg)
{
	int offset;

	msg->status = IOP_MSGSTAT_SENDING;
	offset = IOP_ADDR_SEND_MSG + msg->channel * IOP_MSGLEN;
	_iop_upload(ioph, msg->msg, IOP_MSGLEN, offset);
	iop_write1(ioph, IOP_ADDR_SEND_STATE + msg->channel, IOP_MSG_NEW);

	/* ioph->control_status |= IOP_CS_IRQ; */
	ioph->control_status = (ioph->control_status & 0xfe) | IOP_CS_IRQ;
}

static void
iop_message_sent(IOP *iop, int chan)
{
	IOPHW *ioph;
	struct iop_msg *msg;

	ioph = iop->iop;

	msg = SIMPLEQ_FIRST(&iop->sendq[chan]);
	msg->status = IOP_MSGSTAT_SENT;
	SIMPLEQ_REMOVE_HEAD(&iop->sendq[chan], iopm);

	msg->handler(iop, msg);

	pool_put(&iop->pool, msg);

	if (!(msg = SIMPLEQ_FIRST(&iop->sendq[chan]))) {
		iop_write1(ioph, IOP_ADDR_SEND_STATE + chan, IOP_MSG_IDLE);
	} else {
		load_msg_to_iop(ioph, msg);
	}
}

static void
receive_iop_message(IOP *iop, int chan)
{
	IOPHW *ioph;
	struct iop_msg *msg;
	int offset;

	ioph = iop->iop;

	msg = SIMPLEQ_FIRST(&iop->recvq[chan]);
	if (msg) {
		SIMPLEQ_REMOVE_HEAD(&iop->recvq[chan], iopm);
	} else {
		msg = &iop->unsolicited_msg;
		msg->channel = chan;
		msg->handler = iop->listeners[chan];
		msg->user_data = iop->listener_data[chan];
	}

	offset = IOP_ADDR_RECV_MSG + chan * IOP_MSGLEN;
	_iop_download(ioph, msg->msg, IOP_MSGLEN, offset);
	msg->status = IOP_MSGSTAT_RECEIVED;

	msg->handler(iop, msg);

	if (msg != &iop->unsolicited_msg)
		pool_put(&iop->pool, msg);

	iop_write1(ioph, IOP_ADDR_RECV_STATE + chan, IOP_MSG_COMPLETE);
	ioph->control_status |= IOP_CS_IRQ;

	if ((msg = SIMPLEQ_FIRST(&iop->recvq[chan])) != NULL) {
		msg->status = IOP_MSGSTAT_RECEIVING;
	}
}

int
iop_send_msg(int iopn, int chan, u_char *mesg, int msglen,
    iop_msg_handler handler, void *user_data)
{
	struct iop_msg *msg;
	IOP *iop;
	int s;

	if (iopn & ~1) return -1;
	iop = &mac68k_iops[iopn];
	if (!iop) return -1;
	if (msglen > IOP_MSGLEN) return -1;

	msg = (struct iop_msg *) pool_get(&iop->pool, PR_WAITOK);
	if (msg == NULL) return -1;
printf("have msg buffer for IOP: %#x\n", (unsigned) iop->iop);
	msg->channel = chan;
	if (msglen < IOP_MSGLEN) memset(msg->msg, '\0', IOP_MSGLEN);
	memcpy(msg->msg, mesg, msglen);
	msg->handler = handler;
	msg->user_data = user_data;

	msg->status = IOP_MSGSTAT_QUEUED;

	s = splhigh();
	SIMPLEQ_INSERT_TAIL(&iop->sendq[chan], msg, iopm);
	if (msg == SIMPLEQ_FIRST(&iop->sendq[chan])) {
		msg->status = IOP_MSGSTAT_SENDING;
printf("loading msg to iop: cs: 0x%x V1-%x- ", (unsigned) iop->iop->control_status, (unsigned)via_reg(VIA1, vIFR));
		load_msg_to_iop(iop->iop, msg);
printf("msg loaded to iop: cs: 0x%x V1-%x- ", (unsigned) iop->iop->control_status, (unsigned)via_reg(VIA1, vIFR));
	}

{int i; for (i=0;i<16;i++) {
printf(" cs: 0x%x V1-%x- ", (unsigned) iop->iop->control_status, (unsigned)via_reg(VIA1, vIFR));
delay(1000);
}}
	splx(s);

	return 0;
}

int
iop_queue_receipt(int iopn, int chan, iop_msg_handler handler, void *user_data)
{
	struct iop_msg *msg;
	IOP *iop;
	int s;

	if (iopn & ~1) return -1;
	iop = &mac68k_iops[iopn];
	if (!iop) return -1;

	msg = (struct iop_msg *) pool_get(&iop->pool, PR_WAITOK);
	if (msg == NULL) return -1;
	msg->channel = chan;
	msg->handler = handler;
	msg->user_data = user_data;

	msg->status = IOP_MSGSTAT_QUEUED;

	s = splhigh();
	SIMPLEQ_INSERT_TAIL(&iop->recvq[chan], msg, iopm);
	if (msg == SIMPLEQ_FIRST(&iop->recvq[chan])) {
		msg->status = IOP_MSGSTAT_RECEIVING;
	}
	splx(s);

	return 0;
}

int
iop_register_listener(int iopn, int chan, iop_msg_handler handler,
    void *user_data)
{
	IOP *iop;
	int s;

	if (iopn & ~1) return -1;
	iop = &mac68k_iops[iopn];
	if (!iop) return -1;

	s = splhigh();
	iop->listeners[chan] = handler;
	iop->listener_data[chan] = user_data;
	splx(s);

	return 0;
}
