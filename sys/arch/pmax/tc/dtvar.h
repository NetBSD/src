/*	$NetBSD: dtvar.h,v 1.6 2011/06/04 01:37:36 tsutsui Exp $	*/

/*-
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#ifndef _DTVAR_H_
#define _DTVAR_H_

#define	DT_DEVICE_NO(a)		(((a) - DT_ADDR_FIRST) >> 1)
#define	DT_GET_SHORT(b0, b1)	(((b0) << 8) | (b1))

struct dt_msg {
	uint8_t	dst;
	uint8_t	src;
	uint8_t	ctl;

	/* varzise, checksum byte at end */
	uint8_t	body[DT_MAX_MSG_SIZE-3];

	union {
		SLIST_ENTRY(dt_msg) slist;
		SIMPLEQ_ENTRY(dt_msg) simpleq;
	} chain;
};

#define	DT_CTL(l, s, p)	\
    ((l & 0x1f) | ((s & 0x03) << 5) | ((p & 0x01) << 7))
#define	DT_CTL_P(c)		((c >> 7) & 0x01)
#define	DT_CTL_SUBADDR(c)	((c >> 5) & 0x03)
#define	DT_CTL_LEN(c)		(c & 0x1f)

struct dt_device {
	void	*dtdv_arg;
	void	(*dtdv_handler)(void *, struct dt_msg *);
};

struct dt_state {
	volatile u_int	*ds_data;
	volatile u_int	*ds_poll;
	int		ds_bad_pkts;
	int		ds_state;
	int		ds_escaped;
	int		ds_len;
	int		ds_ptr;
};

struct dt_softc {
	device_t	sc_dev;
	struct dt_msg	sc_msg;
	void		*sc_sih;
	SLIST_HEAD(, dt_msg) sc_free;
	SIMPLEQ_HEAD(, dt_msg) sc_queue;
};

struct dt_attach_args {
	int	dta_addr;
};

#define	DT_GET_ERROR	-1
#define	DT_GET_DONE	0
#define	DT_GET_NOTYET	1

void	dt_cninit(void);
int	dt_identify(int, struct dt_ident *);
int	dt_msg_get(struct dt_msg *, int);
void	dt_msg_dump(struct dt_msg *);
int	dt_establish_handler(struct dt_softc *, struct dt_device *,
    void *, void (*)(void *, struct dt_msg *));

extern int	dt_kbd_addr;
extern struct	dt_device dt_kbd_dv;
extern int	dt_ms_addr;
extern struct	dt_device dt_ms_dv;
extern struct	dt_state dt_state;

#endif	/* !_DTVAR_H_ */
