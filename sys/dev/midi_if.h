/*	$NetBSD: midi_if.h,v 1.17.14.7 2006/05/20 03:32:45 chap Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@NetBSD.org).
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#ifndef _SYS_DEV_MIDI_IF_H_
#define _SYS_DEV_MIDI_IF_H_

struct midi_info {
	char	*name;		/* Name of MIDI hardware */
	int	props;
};
#define MIDI_PROP_OUT_INTR  1
#define MIDI_PROP_CAN_INPUT 2

struct midi_softc;

struct midi_hw_if {
	int	(*open)(void *, int, 	/* open hardware */
			void (*)(void *, int), /* input callback */
			void (*)(void *), /* output callback */
			void *);
	void	(*close)(void *);		/* close hardware */
	int	(*output)(void *, int);	/* output a byte */
	void	(*getinfo)(void *, struct midi_info *);
	int	(*ioctl)(void *, u_long, caddr_t, int, struct proc *);
};

/*
 * The extended hardware interface is for use by drivers that are better off
 * getting messages whole to transmit, rather than byte-by-byte through
 * output().  Two examples are midisyn (which interprets MIDI messages in
 * software to drive synth chips) and umidi (which has to send messages in the
 * packet-based USB MIDI protocol).  It is silly for them to have to reassemble
 * messages midi had to split up to poke through the single-byte interface.
 *
 * To register use of the extended interface, a driver will call back midi's
 * midi_register_hw_if_ext() function during getinfo(); thereafter midi will
 * deliver channel messages, system common messages other than sysex, and sysex
 * messages, respectively, through these methods, and use the original output
 * method only for system realtime messages (all of which are single byte).
 * Other drivers that have no reason to change from the single-byte interface
 * simply don't call the register function, and nothing changes for them.
 */
struct midi_hw_if_ext {
	int	(*channel)(void *, int, int, u_char *, int);
	int	(*common)(void *, int, u_char *, int);
	int	(*sysex)(void *, u_char *, int);
};
void midi_register_hw_if_ext(struct midi_hw_if_ext *);

void	midi_attach(struct midi_softc *, struct device *);
struct device *midi_attach_mi(struct midi_hw_if *, void *, 
				   struct device *);

int	midi_unit_count(void);
void	midi_getinfo(dev_t, struct midi_info *);
int	midi_writebytes(int, u_char *, int);

#if !defined(IPL_AUDIO)
#define splaudio splbio		/* XXX */
#define IPL_AUDIO IPL_BIO	/* XXX */
#endif

#endif /* _SYS_DEV_MIDI_IF_H_ */
