/*	$NetBSD: dtvar.h,v 1.1.2.1 2002/03/15 14:22:51 ad Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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

#ifndef _DTVAR_H_
#define _DTVAR_H_

#define	DT_DEVICE_NO(a)		(((a) - DT_ADDR_FIRST) >> 1)

struct dt_msg {
	u_int8_t	dst;
	u_int8_t	src;
	union {
		struct {
			u_int8_t	len : 5, /* message byte len */
					sub : 2, /* sub-address */
					P : 1;	 /* Ctl(1)/Data(0) marker */
		} val;
		u_int8_t	bits;	/* quick check */
	} code;

	/* varzise, checksum byte at end */
	u_int8_t	body[DT_MAX_MSG_SIZE-3];

	union {
		SLIST_ENTRY(dt_msg) slist;
		SIMPLEQ_ENTRY(dt_msg) simpleq;
	} chain;
};

struct dt_device {
	struct	device *dtdv_dv;
	void	*dtdv_sih;
	SIMPLEQ_HEAD(, dt_msg) dtdv_queue;
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
	struct device	sc_dv;
	struct dt_msg	sc_msg;
	SLIST_HEAD(, dt_msg) sc_free;
	struct dt_device sc_dtdv[(DT_ADDR_DEFAULT - DT_ADDR_FIRST) >> 1];
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
int	dt_establish_handler(struct dt_softc *, int, struct device *,
			     void (*)(void *));

static __inline__ struct	dt_msg *dt_msg_dequeue(struct dt_device *);
static __inline__ void	dt_msg_release(struct dt_softc *, struct dt_msg *);

extern int	dt_kbd_addr;
extern struct	dt_state dt_state;

static __inline__ struct dt_msg *
dt_msg_dequeue(struct dt_device *dtdv)
{
	struct dt_msg *msg;
	int s;

	s = spltty();
	msg = SIMPLEQ_FIRST(&dtdv->dtdv_queue);
	if (msg != NULL)
		SIMPLEQ_REMOVE_HEAD(&dtdv->dtdv_queue, msg, chain.simpleq);
	splx(s);
	return (msg);
}

static __inline__ void
dt_msg_release(struct dt_softc *sc, struct dt_msg *msg)
{
	int s;

	s = spltty();
	SLIST_INSERT_HEAD(&sc->sc_free, msg, chain.slist);
	splx(s);
}

#endif	/* !_DTVAR_H_ */
