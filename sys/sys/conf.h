/*	$NetBSD: conf.h,v 1.101.2.4 2002/06/20 15:53:00 gehenna Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)conf.h	8.5 (Berkeley) 1/9/95
 */

#ifndef _SYS_CONF_H_
#define _SYS_CONF_H_

/*
 * Definitions of device driver entry switches
 */

struct buf;
struct proc;
struct tty;
struct uio;
struct vnode;

/*
 * Types for d_type
 */
#define	D_TAPE	1
#define	D_DISK	2
#define	D_TTY	3

/*
 * Block device switch table
 */
struct bdevsw {
	int		(*d_open)(dev_t, int, int, struct proc *);
	int		(*d_close)(dev_t, int, int, struct proc *);
	void		(*d_strategy)(struct buf *);
	int		(*d_ioctl)(dev_t, u_long, caddr_t, int, struct proc *);
	int		(*d_dump)(dev_t, daddr_t, caddr_t, size_t);
	int		(*d_psize)(dev_t);
	int		d_type;
};

/*
 * Character device switch table
 */
struct cdevsw {
	int		(*d_open)(dev_t, int, int, struct proc *);
	int		(*d_close)(dev_t, int, int, struct proc *);
	int		(*d_read)(dev_t, struct uio *, int);
	int		(*d_write)(dev_t, struct uio *, int);
	int		(*d_ioctl)(dev_t, u_long, caddr_t, int, struct proc *);
	void		(*d_stop)(struct tty *, int);
	struct tty *	(*d_tty)(dev_t);
	int		(*d_poll)(dev_t, int, struct proc *);
	paddr_t		(*d_mmap)(dev_t, off_t, int);
	int		d_type;
};

#ifdef _KERNEL

int devsw_attach(const char *, const struct bdevsw *, int *,
		 const struct cdevsw *, int *);
void devsw_detach(const struct bdevsw *, const struct cdevsw *);
const struct bdevsw *bdevsw_lookup(dev_t);
const struct cdevsw *cdevsw_lookup(dev_t);
int bdevsw_lookup_major(const struct bdevsw *);
int cdevsw_lookup_major(const struct cdevsw *);

#define	dev_type_open(n)	int n (dev_t, int, int, struct proc *)
#define	dev_type_close(n)	int n (dev_t, int, int, struct proc *)
#define	dev_type_read(n)	int n (dev_t, struct uio *, int)
#define	dev_type_write(n)	int n (dev_t, struct uio *, int)
#define	dev_type_ioctl(n) \
		int n (dev_t, u_long, caddr_t, int, struct proc *)
#define	dev_type_stop(n)	void n (struct tty *, int)
#define	dev_type_tty(n)		struct tty * n (dev_t)
#define	dev_type_poll(n)	int n (dev_t, int, struct proc *)
#define	dev_type_mmap(n)	paddr_t n (dev_t, off_t, int)
#define	dev_type_strategy(n)	void n (struct buf *)
#define	dev_type_dump(n)	int n (dev_t, daddr_t, caddr_t, size_t)
#define	dev_type_size(n)	int n (dev_t)

#define	noopen		((dev_type_open((*)))enodev)
#define	noclose		((dev_type_close((*)))enodev)
#define	noread		((dev_type_read((*)))enodev)
#define	nowrite		((dev_type_write((*)))enodev)
#define	noioctl		((dev_type_ioctl((*)))enodev)
#define	nostop		((dev_type_stop((*)))enodev)
#define	notty		NULL
#define	nopoll		seltrue
#define	nommap		((dev_type_mmap((*)))enodev)
#define	nodump		((dev_type_dump((*)))enodev)
#define	nosize		NULL

#define	nullopen	((dev_type_open((*)))nullop)
#define	nullclose	((dev_type_close((*)))nullop)
#define	nullread	((dev_type_read((*)))nullop)
#define	nullwrite	((dev_type_write((*)))nullop)
#define	nullioctl	((dev_type_ioctl((*)))nullop)
#define	nullstop	((dev_type_stop((*)))nullop)
#define	nullpoll	((dev_type_poll((*)))nullop)
#define	nullmmap	((dev_type_mmap((*)))nullop)
#define	nulldump	((dev_type_dump((*)))nullop)

/* symbolic sleep message strings */
extern	const char devopn[], devio[], devwait[], devin[], devout[];
extern	const char devioc[], devcls[];

#endif /* _KERNEL */

/*
 * Line discipline switch table
 */
struct linesw {
	char	*l_name;	/* Linesw name */
	int	l_no;		/* Linesw number (compatibility) */

	int	(*l_open)	__P((dev_t dev, struct tty *tp));
	int	(*l_close)	__P((struct tty *tp, int flags));
	int	(*l_read)	__P((struct tty *tp, struct uio *uio,
				     int flag));
	int	(*l_write)	__P((struct tty *tp, struct uio *uio,
				     int flag));
	int	(*l_ioctl)	__P((struct tty *tp, u_long cmd, caddr_t data,
				     int flag, struct proc *p));
	int	(*l_rint)	__P((int c, struct tty *tp));
	int	(*l_start)	__P((struct tty *tp));
	int	(*l_modem)	__P((struct tty *tp, int flag));
	int	(*l_poll)	__P((struct tty *tp, int events,
				     struct proc *p));
};

#ifdef _KERNEL
extern struct linesw **linesw;
extern int nlinesw;
extern void ttyldisc_init __P((void));
int ttyldisc_add __P((struct linesw *disc, int no));
struct linesw *ttyldisc_remove __P((char *name));
struct linesw *ttyldisc_lookup __P((char *name));

/* For those defining their own line disciplines: */
#define	ttynodisc ((int (*) __P((dev_t, struct tty *)))enodev)
#define	ttyerrclose ((int (*) __P((struct tty *, int flags)))enodev)
#define	ttyerrio ((int (*) __P((struct tty *, struct uio *, int)))enodev)
#define	ttyerrinput ((int (*) __P((int c, struct tty *)))enodev)
#define	ttyerrstart ((int (*) __P((struct tty *)))enodev)
#define	ttyerrpoll ((int (*) __P((struct tty *, int, struct proc *)))enodev)

int	ttynullioctl __P((struct tty *, u_long, caddr_t, int, struct proc *));
#endif

/*
 * Swap device table
 */
struct swdevt {
	dev_t	sw_dev;
	int	sw_flags;
	int	sw_nblks;
	struct	vnode *sw_vp;
};
#define	SW_FREED	0x01
#define	SW_SEQUENTIAL	0x02
#define	sw_freed	sw_flags	/* XXX compat */

#ifdef _KERNEL
extern struct swdevt swdevt[];

extern const int mem_no;
int	iskmemdev __P((dev_t));

#define	DEV_MEM		0	/* minor device 0 is physical memory */
#define	DEV_KMEM	1	/* minor device 1 is kernel memory */
#define DEV_NULL	2	/* minor device 2 is EOF/rathole */
#ifdef __arm__			/* XXX: FIX ME ARM! */
#define DEV_ZERO	3	/* minor device 3 is '\0'/rathole */
#else
#define DEV_ZERO	12	/* minor device 12 is '\0'/rathole */
#endif

#endif /* _KERNEL */

struct devsw_conv {
	const char *d_name;
	int d_bmajor;
	int d_cmajor;
};

#ifdef _KERNEL
const char *devsw_blk2name(int);
int devsw_name2blk(const char *, char *, size_t);
dev_t devsw_chr2blk(dev_t);
dev_t devsw_blk2chr(dev_t);
#endif /* _KERNEL */

#ifdef _KERNEL
struct	device;
void	setroot __P((struct device *, int));
void	swapconf __P((void));
#endif /* _KERNEL */

#endif /* !_SYS_CONF_H_ */
