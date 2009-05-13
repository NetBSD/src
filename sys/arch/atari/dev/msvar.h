/*	$NetBSD: msvar.h,v 1.2.148.1 2009/05/13 17:16:22 jym Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman
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
 *	This product includes software developed by Leo Weppelman.
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
 */
#ifndef _MSVAR_H
#define _MSVAR_H

/*
 * Mouse specific packages produced by the keyboard. Currently, we only
 * define the REL_MOUSE package, as this is the only one used.
 */
typedef struct {
	u_char	id;
	char	dx;
	char	dy;
} REL_MOUSE;

#define IS_REL_MOUSE(id)	(((u_int)(id) & 0xF8) == 0xF8)
#define TIMEOUT_ID		(0xFC)

struct ms_softc {
	u_char			ms_buttons; /* button states		*/
	u_char			ms_emul3b;  /* emulate 3rd button	*/
	struct	evvar		ms_events;  /* event queue state	*/
	int			ms_dx;	    /* accumulated dx		*/
	int			ms_dy;	    /* accumulated dy		*/
	struct firm_event	ms_bq[2];   /* Button queue		*/
	int			ms_bq_idx;  /* Button queue index	*/
	struct callout		ms_delay_ch;
};

#ifdef _KERNEL
void	mouse_soft(REL_MOUSE *, int, int);
int	mouseattach(int);
#endif /* _KERNEL */

#endif /* _MSVAR_H */
