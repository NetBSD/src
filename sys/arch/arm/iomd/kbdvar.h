/*	$NetBSD: kbdvar.h,v 1.1 2001/10/05 22:27:41 reinoud Exp $	*/

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
 * kbdvar.h
 *
 * Created      : 02/02/97
 */

/* Define the key_struct structure */

typedef struct {
	int base_code;	/* Base ASCII code */
	int shift_code;	/* Shifted ASCII code */
	int ctrl_code;	/* CTRL code */
	int alt_code;	/* Alt code */
	int flags;	/* Flags field */
} key_struct;

/*
 * softc structure for the kbd device
 */

struct kbd_softc {
	struct device	sc_device;	/* device node */
	void		*sc_ih;		/* interrupt pointer */

	bus_space_tag_t	sc_iot;		/* bus tag */
	bus_space_handle_t sc_ioh;	/* bus handle */

	int sc_state;			/* device state */
#define RAWKBD_OPEN	0x01
#define KBD_OPEN	0x02
#define RAWKBD_ASLEEP	0x04
	struct clist	sc_q;
	struct selinfo	sc_rsel;
	struct proc	*sc_proc;
};

/* function prototypes used by attach routines */

int kbdreset __P((struct kbd_softc *sc));
int kbdintr __P((void *arg));

/* End of kbdvar.h */
