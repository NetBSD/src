/*	$NetBSD: qvavar.h,v 1.1.2.2 2015/09/22 12:05:53 skrll Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles H. Dickman
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

/*
 * Copyright (c) 1996, 1997 Philip L. Budne.
 * Copyright (c) 1993 Philip A. Nelson.
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
 *	This product includes software developed by Philip A. Nelson.
 * 4. The name of Philip A. Nelson may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PHILIP NELSON ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL PHILIP NELSON BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	qvavar.h: definitions for qvss scn2681 DUART
 */

#ifndef _VAX_UBA_QVVAR_H_
#define _VAX_UBA_QVVAR_H_

/* Constants. */
#ifdef COMDEF_SPEED
#undef  TTYDEF_SPEED
#define TTYDEF_SPEED    COMDEF_SPEED	/* default baud rate */
#endif

#define SCN_SIZE	         0x10	/* address space for port */
#define SCN_CONSOLE		   0	/* minor number of console */
#define SCN_CONSDUART		   0
#define SCN_CONSCHAN		   0

#define CH_SZ		16
#define DUART_SZ	32
#define SCN_REG(n)	((n) << 1)	/* DUART bytes are word aligned */
#define QVA_FIRSTREG    SCN_REG(0)
#define QVA_WINSIZE     (SCN_REG(16) - SCN_REG(0))

/* A QVAUX is SCN2681 DUART */

#define	NQVAUXLINE 	2

#define QVA_C2I(c)	(0)	          /* convert controller # to index */
#define QVA_I2C(c)	(0)	          /* convert minor to controller # */
#define QVA_PORT(u)	((u)&01)	  /* extract the port # */
#define QVA_QVCSR       (-32)             /* offset to VCB01 CSR */
#define QVA_QVIC        (QVA_QVCSR + 12)  /* offset to VCB01 IC */

struct	qvaux_softc {
	device_t	    sc_dev;	  /* Autoconf blaha */
	struct	evcnt	    sc_rintrcnt;  /* recevive interrupt counts */
	struct	evcnt	    sc_tintrcnt;  /* transmit interrupt counts */
	struct	qvaux_regs  sc_qr;	  /* reg pointers */
	bus_space_tag_t	    sc_iot;
	bus_space_handle_t  sc_ioh;
	int		    sc_consline;  /* console line, or -1 XXX */
	int		    sc_rxint;     /* Receive interrupt count XXX */
	u_char		    sc_brk;	     /* Break asserted on some lines */
	u_char		    sc_dsr;	     /* DSR set bits if no mdm ctrl */
	int                 sc_imr;      /* interrupts that are enabled */
	struct qvaux_linestate {
		struct qvaux_softc *qvaux_sc;	/* backpointer to softc */
		int	    qvaux_line;		/* channel number */
		void	    *qvaux_private;	/* sub-driver data pointer */
		int	    (*qvaux_catch)(void *, int); /* Fast catch recv */
		struct tty  *qvaux_tty;		/* what we work on */
#ifdef notyet
		void *	    qvaux_mem;		/* pointers to clist output */
		void *	    qvaux_end;		/*   allowing pdma action */
#endif
	} sc_qvaux[NQVAUXLINE];
};

void	qvauxattach(struct qvaux_softc *, struct evcnt *, int);
void    qvauxint(void *);
void	qvauxrint(void *);
void	qvauxxint(void *);
void	qvauxreset(device_t);

#endif /* _VAX_UBA_QVVAR_H_ */

