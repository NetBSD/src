/*	$NetBSD: qmsvar.h,v 1.2 2002/02/18 18:43:55 bjh21 Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited
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
 *	This product includes software developed by Mark Brinicombe
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * qmsvar.h
 *
 * Created      : 02/02/97
 */

/* 
 * softc structure for the qms device
 */

struct qms_softc {
	struct device sc_device;		/* device node */
	void *sc_ih;				/* interrupt pointer */
	bus_space_tag_t	sc_iot;			/* bus tag */
	bus_space_handle_t sc_ioh;		/* bus handle */
	bus_space_handle_t sc_butioh;		/* bus handle */
	int sc_irqnum;				/* interrupt number */

	void (*sc_intenable) __P((struct qms_softc *, int));

	struct selinfo sc_rsel;
#define QMOUSE_OPEN 0x01
#define QMOUSE_ASLEEP 0x02
	int sc_state;				/* device state */
	int sc_mode;				/* device mode */
	int boundx, boundy, bounda, boundb;	/* Bounding box.  x,y is bottom left */
	int origx, origy;
	int xmult, ymult;	/* Multipliers */
	int lastx, lasty, lastb;
	struct proc *sc_proc;
	struct clist sc_buffer;
};

/* function prototypes used by attach routines */

int  qmsintr	__P((void *arg));
void qmsattach	__P((struct qms_softc *sc));

#ifdef DIAGNOSTIC
void qms_console_freeze __P((void));
#endif

/* End of qmsvar.h */
