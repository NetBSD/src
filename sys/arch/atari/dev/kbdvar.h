/*	$NetBSD: kbdvar.h,v 1.3.36.1 2001/09/13 01:13:20 thorpej Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.
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
 *      This product includes software developed by Leo Weppelman.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef _KBDVAR_H
#define _KBDVAR_H

/*
 * The ringbuffer is the interface between the hard and soft interrupt handler.
 * The hard interrupt runs straight from the MFP interrupt.
 */
#define KBD_RING_SIZE	256   /* Sz of input ring buffer, must be power of 2 */
#define KBD_RING_MASK	255   /* Modulo mask for above			     */

struct kbd_softc {
	int		k_event_mode;	/* if 1, collect events,	*/
					/*   else pass to ite		*/
	struct evvar	k_events;	/* event queue state		*/
	u_char		k_soft_cs;	/* control-reg. copy		*/
	u_char		k_package[20];	/* XXX package being build	*/
	u_char		k_pkg_size;	/* Size of the package		*/
	u_char		k_pkg_idx;	/* Running pkg assembly index	*/
	u_char		k_pkg_type;	/* Type of package		*/
	u_char		*k_sendp;	/* Output pointer		*/
	int		k_send_cnt;	/* Chars left for output	*/
};

/*
 * Package types
 */
#define	KBD_MEM_PKG	0		/* Memory read package		*/
#define	KBD_AMS_PKG	1		/* Absolute mouse package	*/
#define	KBD_RMS_PKG	2		/* Relative mouse package	*/
#define	KBD_CLK_PKG	3		/* Clock package		*/
#define	KBD_JOY0_PKG	4		/* Joystick-0 package		*/
#define	KBD_JOY1_PKG	5		/* Joystick-1 package		*/
#define	KBD_TIMEO_PKG	6		/* Timeout package		*/

#ifdef _KERNEL
extern	u_char	kbd_modifier;

void	kbd_bell_gparms __P((u_int *, u_int *, u_int *));
void	kbd_bell_sparms __P((u_int, u_int, u_int));
void	kbd_write __P((u_char *, int));
int	kbdgetcn __P((void));
void	kbdbell __P((void));
void	kbdenable __P((void));
#endif /* _KERNEL */

#endif /* _KBDVAR_H */
