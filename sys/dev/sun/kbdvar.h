/*	$NetBSD: kbdvar.h,v 1.9 2002/10/03 16:13:26 uwe Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kbd.c	8.2 (Berkeley) 10/30/93
 */

struct kbd_softc {
	struct device k_dev;	/* required first: base device */

	/* middle layer methods */
	struct kbd_ops *k_ops;

	/* state of the upper layer */
	int k_evmode;		/* set if we should produce events */
	struct evvar k_events;	/* event queue state */

	/* ACSII translation state */
	struct kbd_state k_state;

	/* Console hooks */
	int k_isconsole;
	struct cons_channel *k_cc;
};

/* 
 * Downcalls to the middle layer.
 * NB: iocsled has user context to care about,
 *     setleds can be called from interrupt handler.
 */
struct kbd_ops {
	int (*open)(struct kbd_softc *);
	int (*close)(struct kbd_softc *);
	int (*docmd)(struct kbd_softc *, int, int);	/* KIOCCMD */
	int (*setleds)(struct kbd_softc *, int, int);	/* KIOCSLED */
};


/* Callbacks for middle layer to feed us keyboard input */
extern int	kbd_input_keysym(struct kbd_softc *, int);	/* console */
extern void	kbd_input_event(struct kbd_softc *, int);	/* events  */


/*
 * kbd console input channel interface.
 * XXX - does not belong in this header; but for now, kbd is the only user..
 */
struct cons_channel {
	/* XXX: only used by PROM console, probably belongs to kd.c */
	struct callout cc_callout;

	/*
	 * Callbacks provided by underlying device (e.g. keyboard driver).
	 * Console driver will call these before console is opened/closed.
	 */
	void *cc_dev;		/* underlying device private data */
	int (*cc_iopen)(struct cons_channel *);  /* open underlying device */
	int (*cc_iclose)(struct cons_channel *); /* close underlying device */

	/*
	 * Callback provided by the console driver.  Underlying driver
	 * calls it to pass input character up as console input.
	 */
	void (*cc_upstream)(int);
};

/* upper layer callbacks that lower layer passes to console driver */
extern int kbd_cc_open(struct cons_channel *);
extern int kbd_cc_close(struct cons_channel *);

/* Special hook to attach the keyboard driver to the console */
struct consdev;
extern void cons_attach_input(struct cons_channel *, struct consdev *);
