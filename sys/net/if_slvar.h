/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 *	from: @(#)if_slvar.h	7.7 (Berkeley) 5/7/91
 *	$Id: if_slvar.h,v 1.7 1993/12/20 07:47:18 cgd Exp $
 */

#ifndef _IF_SLVAR_H_
#define _IF_SLVAR_H_

/*
 * Definitions for SLIP interface data structures
 * 
 * (This exists so programs like slstats can get at the definition
 *  of sl_softc.)
 */
struct sl_softc {
	struct	ifnet sc_if;		/* network-visible interface */
	struct	ifqueue sc_fastq;	/* interactive output queue */
	struct	tty *sc_ttyp;		/* pointer to tty structure */
	u_char	*sc_mp;			/* pointer to next available buf char */
	u_char	*sc_ep;			/* pointer to last available buf char */
	u_char	*sc_buf;		/* input buffer */
	u_int	sc_flags;		/* see below */
	u_int	sc_escape;	/* =1 if last char input was FRAME_ESCAPE */
	long	sc_lasttime;		/* last time a char arrived */
	long	sc_abortcount;		/* number of abort esacpe chars */
	long	sc_starttime;		/* time of first abort in window */
#ifdef INET				/* XXX */
	struct	slcompress sc_comp;	/* tcp compression data */
#endif
	caddr_t	sc_bpf;			/* BPF data */
};

/* internal flags */
#define	SC_ERROR	0x0001		/* had an input error */

/* visible flags */
#define	SC_COMPRESS	IFF_LINK0	/* compress TCP traffic */
#define	SC_NOICMP	IFF_LINK1	/* supress ICMP traffic */
#define	SC_AUTOCOMP	IFF_LINK2	/* auto-enable TCP compression */

/* this stuff doesn't belong here... */
#define	SLIOCGUNIT	_IOR('t', 88, int)	/* get slip unit number */

/*
 * definitions of the pseudo- link-level header attached to  slip
 * packets grabbed by the packet filter (bpf) traffic monitor.
 * These definitions pulled from BPF's "slip.h" by cgd.
 */
#define	SLIP_HDRLEN	16		/* BPF SLIP header length */

/* offsets into BPF SLIP header */
#define	SLX_DIR		0		/* direction; see below */
#define	SLX_CHDR	1		/* compressed header data */
#define	CHDR_LEN	15		/* length of compressed header data */

#define	SLIPDIR_IN	0		/* incoming */
#define	SLIPDIR_OUT	1		/* outgoing */

#ifdef KERNEL
void	slattach __P((void));
void	slclose __P((struct tty *));
void	slinput __P((int, struct tty *));
int	slioctl __P((struct ifnet *, int, caddr_t));
int	slopen __P((dev_t, struct tty *));
int	sloutput __P((struct ifnet *,
	    struct mbuf *, struct sockaddr *, struct rtentry *));
void	slstart __P((struct tty *));
int	sltioctl __P((struct tty *, int, caddr_t, int));
#endif /* KERNEL */
#endif /* !_IF_SLVAR_H_ */
