/*	$NetBSD: conf.h,v 1.88.2.2 2001/09/13 01:16:27 thorpej Exp $	*/

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
struct knote;
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

#ifdef _KERNEL

#define	dev_type_open(n)	int n __P((dev_t, int, int, struct proc *))
#define	dev_type_close(n)	int n __P((dev_t, int, int, struct proc *))
#define	dev_type_strategy(n)	void n __P((struct buf *))
#define	dev_type_ioctl(n) \
	int n __P((dev_t, u_long, caddr_t, int, struct proc *))

#define	dev_decl(n,t)		__CONCAT(dev_type_,t)(__CONCAT(n,t))
#define	dev_init(c,n,t) \
	((c) > 0 ? __CONCAT(n,t) : (__CONCAT(dev_type_,t)((*))) enxio)
#define	dev_noimpl(t,f)		(__CONCAT(dev_type_,t)((*)))f

#endif /* _KERNEL */

/*
 * Block device switch table
 */
struct bdevsw {
	int	(*d_open)	__P((dev_t dev, int oflags, int devtype,
				     struct proc *p));
	int	(*d_close)	__P((dev_t dev, int fflag, int devtype,
				     struct proc *p));
	void	(*d_strategy)	__P((struct buf *bp));
	int	(*d_ioctl)	__P((dev_t dev, u_long cmd, caddr_t data,
				     int fflag, struct proc *p));
	int	(*d_dump)	__P((dev_t dev, daddr_t blkno, caddr_t va,
				    size_t size));
	int	(*d_psize)	__P((dev_t dev));
	int	d_type;
};

#ifdef _KERNEL

extern struct bdevsw bdevsw[];

/* bdevsw-specific types */
#define	dev_type_dump(n)	int n __P((dev_t, daddr_t, caddr_t, size_t))
#define	dev_type_size(n)	int n __P((dev_t))

/* bdevsw-specific initializations */
#define	dev_size_init(c,n)	(c > 0 ? __CONCAT(n,size) : 0)

#define	bdev_decl(n) \
	dev_decl(n,open); dev_decl(n,close); dev_decl(n,strategy); \
	dev_decl(n,ioctl); dev_decl(n,dump); dev_decl(n,size)

#define	bdev_disk_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), \
	dev_init(c,n,strategy), dev_init(c,n,ioctl), \
	dev_init(c,n,dump), dev_size_init(c,n), D_DISK }

#define	bdev_tape_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), \
	dev_init(c,n,strategy), dev_init(c,n,ioctl), \
	dev_init(c,n,dump), 0, D_TAPE }

#define	bdev_swap_init(c,n) { \
	dev_noimpl(open,enodev), dev_noimpl(close,enodev), \
	dev_init(c,n,strategy), dev_noimpl(ioctl,enodev), \
	dev_noimpl(dump,enodev), 0 }

#ifdef LKM
#define	bdev_lkm_dummy() { \
	dev_noimpl(open,lkmenodev), dev_noimpl(close,enodev), \
	dev_noimpl(strategy,enodev), dev_noimpl(ioctl,enodev), \
	dev_noimpl(dump,enodev), 0 }
#else
#define	bdev_lkm_dummy()	bdev_notdef()
#endif

#define	bdev_notdef() { \
	dev_noimpl(open,enodev), dev_noimpl(close,enodev), \
	dev_noimpl(strategy,enodev), dev_noimpl(ioctl,enodev), \
	dev_noimpl(dump,enodev), 0 }

#endif /* _KERNEL */

/*
 * Character device switch table
 */
struct cdevsw {
	int	(*d_open)	__P((dev_t dev, int oflags, int devtype,
				     struct proc *p));
	int	(*d_close)	__P((dev_t dev, int fflag, int devtype,
				     struct proc *));
	int	(*d_read)	__P((dev_t dev, struct uio *uio, int ioflag));
	int	(*d_write)	__P((dev_t dev, struct uio *uio, int ioflag));
	int	(*d_ioctl)	__P((dev_t dev, u_long cmd, caddr_t data,
				     int fflag, struct proc *p));
	void	(*d_stop)	__P((struct tty *tp, int rw));
	struct tty *
		(*d_tty)	__P((dev_t dev));
	int	(*d_poll)	__P((dev_t dev, int events, struct proc *p));
	paddr_t	(*d_mmap)	__P((dev_t, off_t, int));
	int	(*d_kqfilter)	__P((dev_t dev, struct knote *kn));
	int	d_type;
};

#ifdef _KERNEL

extern struct cdevsw cdevsw[];

/* cdevsw-specific types */
#define	dev_type_read(n)	int n __P((dev_t, struct uio *, int))
#define	dev_type_write(n)	int n __P((dev_t, struct uio *, int))
#define	dev_type_stop(n)	void n __P((struct tty *, int))
#define	dev_type_tty(n)		struct tty *n __P((dev_t))
#define	dev_type_poll(n)	int n __P((dev_t, int, struct proc *))
#define	dev_type_kqfilter(n)	int n __P((dev_t, struct knote *))
#define	dev_type_mmap(n)	paddr_t n __P((dev_t, off_t, int))

#define	cdev_decl(n) \
	dev_decl(n,open); dev_decl(n,close); dev_decl(n,read); \
	dev_decl(n,write); dev_decl(n,ioctl); dev_decl(n,stop); \
	dev_decl(n,tty); dev_decl(n,poll); dev_decl(n,mmap); \
	dev_decl(n,kqfilter)

/* helper macros */

/*  open, close, ioctl */
#define	cdev__oci_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_noimpl(read,enodev), \
	dev_noimpl(write,enodev), dev_init(c,n,ioctl), \
	dev_noimpl(stop,enodev), 0, seltrue, dev_noimpl(mmap,enodev), \
	dev_noimpl(kqfilter,enodev), 0 }

/*  open, close, ioctl, poll */
#define cdev__ocip_init(c,n) { \
        dev_init(c,n,open), dev_init(c,n,close), dev_noimpl(read,enodev), \
        dev_noimpl(write,enodev), dev_init(c,n,ioctl), \
        dev_noimpl(stop,enodev), 0, dev_init(c,n,poll), \
	dev_noimpl(mmap,enodev), dev_init(c,n,kqfilter), 0 }

/*  open, close, ioctl, poll, mmap */
#define cdev__ocipm_init(c,n) { \
        dev_init(c,n,open), dev_init(c,n,close), dev_noimpl(read,enodev), \
        dev_noimpl(write,enodev), dev_init(c,n,ioctl), \
        dev_noimpl(stop,enodev), 0, dev_init(c,n,poll), \
	dev_init(c,n,mmap), dev_init(c,n,kqfilter), 0 }

/*  open, close, read, ioctl */
#define	cdev__ocri_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_noimpl(write,enodev), dev_init(c,n,ioctl), \
	dev_noimpl(stop,enodev), 0, seltrue, dev_noimpl(mmap,enodev), \
	dev_noimpl(kqfilter,enodev), 0 }

/*  open, close, read, write */
#define	cdev__ocrw_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_noimpl(ioctl,enodev), \
	dev_noimpl(stop,enodev), 0, seltrue, dev_noimpl(mmap,enodev), \
	dev_noimpl(kqfilter,enodev), 0 }

/*  open, close, read, write, ioctl */
#define	cdev__ocrwi_t_init(c,n,t) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), dev_noimpl(stop,enodev), \
	0, seltrue, dev_noimpl(mmap,enodev), dev_noimpl(kqfilter,enodev), t }
#define	cdev__ocrwi_init(c,n)	cdev__ocrwi_t_init(c,n,0)

/*  open, close, read, write, ioctl, mmap */
#define	cdev__ocrwim_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), dev_noimpl(stop,enodev), \
	0, seltrue, dev_init(c,n,mmap), dev_noimpl(kqfilter,enodev), 0 }

/*  open, close, read, ioctl, poll */
#define	cdev__ocrip_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_noimpl(write,enodev), dev_init(c,n,ioctl), \
	dev_noimpl(stop,enodev), 0, dev_init(c,n,poll), \
	dev_noimpl(mmap,enodev), dev_init(c,n,kqfilter), 0 }

/*  open, close, read, write, poll */
#define	cdev__ocrwp_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_noimpl(ioctl,enodev), \
	dev_noimpl(stop,enodev), 0, dev_init(c,n,poll), \
	dev_noimpl(mmap,enodev), dev_init(c,n,kqfilter), 0 }

/*  open, close, read, write, ioctl, poll */
#define cdev__ocrwip_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), dev_noimpl(stop,enodev), \
	0, dev_init(c,n,poll), dev_noimpl(mmap,enodev), \
	dev_init(c,n,kqfilter), 0 }

/*  open, close, (read), (write), ioctl, poll, mmap */
#define	cdev__ocRWipm_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), \
	dev_noimpl(read,nullop), dev_noimpl(write,nullop), \
	dev_init(c,n,ioctl), dev_noimpl(stop,enodev), 0, \
	dev_init(c,n,poll), dev_init(c,n,mmap), dev_init(c,n,kqfilter), 0 }

/*  open, close, ioctl, mmap */
#define	cdev__ocim_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), \
	dev_noimpl(read,enodev), dev_noimpl(write,enodev), \
	dev_init(c,n,ioctl), dev_noimpl(stop,enodev), 0, \
	dev_noimpl(poll,enodev), dev_init(c,n,mmap), 0 }

/*  open, close, read, write, ioctl, stop, poll */
#define	cdev__ocrwisp_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), dev_init(c,n,stop), \
	0, dev_init(c,n,poll), dev_noimpl(mmap,enodev), \
	dev_init(c,n,kqfilter), 0 }

/*  open, close, write, ioctl */
#define	cdev__ocwi_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_noimpl(read,enodev), \
	dev_init(c,n,write), dev_init(c,n,ioctl), dev_noimpl(stop,enodev), \
	0, seltrue, dev_noimpl(mmap,enodev), dev_noimpl(kqfilter,enodev), 0 }

/*  open, close, read, write, ioctl, stop, tty, poll, mmap, kqfilter */
#define	cdev__ttym_init(c,n,t) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), dev_init(c,n,stop), \
	dev_init(c,n,tty), dev_init(c,n,poll), dev_init(c,n,mmap), \
	dev_init(c,n,kqfilter), t }

/* open, close, read, write, ioctl */
#define	cdev_disk_init(c,n)	cdev__ocrwi_t_init(c,n,D_DISK)
#define	cdev_tape_init(c,n)	cdev__ocrwi_t_init(c,n,D_TAPE)

/* open, close, read, write, ioctl, stop, tty, poll, ttykqfilter */
#define	cdev_tty_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), dev_init(c,n,stop), \
	dev_init(c,n,tty), dev_init(c,n,poll), dev_noimpl(mmap,enodev), \
	ttykqfilter, D_TTY }

/* open, close, read, ioctl, poll */
#define	cdev_mouse_init(c,n)	cdev__ocrip_init(c,n)

/* open, close, read, ioctl */
#define cdev_scanner_init(c,n)	cdev__ocri_init(c,n)

#ifdef LKM
#define	cdev_lkm_dummy() { \
	dev_noimpl(open,lkmenodev), dev_noimpl(close,enodev), \
	dev_noimpl(read,enodev), dev_noimpl(write,enodev), \
	dev_noimpl(ioctl,enodev), dev_noimpl(stop,enodev), \
	0, seltrue, dev_noimpl(mmap,enodev), dev_noimpl(kqfilter,enodev), 0 }
#else
#define	cdev_lkm_dummy()	cdev_notdef()
#endif

#define	cdev_notdef() { \
	dev_noimpl(open,enodev), dev_noimpl(close,enodev), \
	dev_noimpl(read,enodev), dev_noimpl(write,enodev), \
	dev_noimpl(ioctl,enodev), dev_noimpl(stop,enodev), \
	0, seltrue, dev_noimpl(mmap,enodev), dev_noimpl(kqfilter,enodev), 0 }

/* open, close, read, write, ioctl, stop, poll, kqfilter -- XXX should be a tty
but needs own kqfilter */
#define	cdev_cn_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), dev_init(c,n,stop), \
	0, dev_init(c,n,poll), dev_noimpl(mmap,enodev), \
	dev_init(c,n,kqfilter), D_TTY }

/* open, (close), read, write, ioctl, (stop), poll -- XXX should be a tty */
#define cdev_ctty_init(c,n) { \
	dev_init(c,n,open), dev_noimpl(close,nullop), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), dev_noimpl(stop,nullop), \
	0, dev_init(c,n,poll), dev_noimpl(mmap,enodev), \
	dev_init(c,n,kqfilter), D_TTY }

/* open, close, read, write, mmap */
#define cdev_mm_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_noimpl(ioctl,enodev), \
	dev_noimpl(stop,enodev), 0, seltrue, dev_init(c,n,mmap), \
	dev_noimpl(kqfilter,enodev), 0 }

/* (open), (close), read, write */
#define cdev_swap_init(c,n) { \
	dev_noimpl(open,nullop), dev_noimpl(close,nullop), \
	dev_init(c,n,read), dev_init(c,n,write), dev_noimpl(ioctl,enodev), \
	dev_noimpl(stop,enodev), 0, seltrue, dev_noimpl(mmap,enodev), \
	dev_noimpl(kqfilter,enodev), 0 }

/* open, close, read, write, ioctl, (stop), tty, poll, kqfilter */
#define cdev_ptc_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), dev_noimpl(stop,nullop), \
	dev_init(c,n,tty), dev_init(c,n,poll), dev_noimpl(mmap,enodev), \
	dev_init(c,n,kqfilter), D_TTY }

/* open, close, read, ioctl, poll -- XXX should be a generic device */
#define cdev_log_init(c,n)	cdev__ocrip_init(c,n)
#define	cdev_ch_init(c,n)	cdev__ocrip_init(c,n)

/* open */
#define cdev_fd_init(c,n) { \
	dev_init(c,n,open), dev_noimpl(close,enodev), \
	dev_noimpl(read,enodev), dev_noimpl(write,enodev), \
	dev_noimpl(ioctl,enodev), dev_noimpl(stop,enodev), \
	0, seltrue, dev_noimpl(mmap,enodev), dev_noimpl(kqfilter,enodev), 0 }

/* open, close, read, write, ioctl, poll -- XXX should be generic device */
#define cdev_bpftun_init(c,n)	cdev__ocrwip_init(c,n)

/* open, close, ioctl */
#define	cdev_lkm_init(c,n)	cdev__oci_init(c,n)
#define	cdev_uk_init(c,n)	cdev__oci_init(c,n)
#define	cdev_scsibus_init(c,n)	cdev__oci_init(c,n)
#define	cdev_se_init(c,n)	cdev__oci_init(c,n)
#define	cdev_ses_init(c,n)	cdev__oci_init(c,n)
#define	cdev_sysmon_init(c,n)	cdev__oci_init(c,n)
#define	cdev_openfirm_init(c,n)	cdev__oci_init(c,n)
#define	cdev_openprom_init(c,n)	cdev__oci_init(c,n)

/* open, close, read, ioctl, poll */
#define	cdev_usb_init(c,n)	cdev__ocrip_init(c,n)

/* open, close, read, write, ioctl, poll */
#define cdev_rnd_init(c,n)	cdev__ocrwip_init(c,n)
#define	cdev_usbdev_init(c,n)	cdev__ocrwip_init(c,n)
#define	cdev_ugen_init(c,n)	cdev__ocrwip_init(c,n)
#define cdev_midi_init(c,n)	cdev__ocrwip_init(c,n)

/* open, close, ioctl, poll, mmap */
#define	cdev_fb_init(c,n)	cdev__ocipm_init(c,n)

/* open, close, read, write, ioctl, poll, mmap */
#define cdev_audio_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), dev_noimpl(stop,enodev), \
	0, dev_init(c,n,poll), dev_init(c,n,mmap), \
	dev_init(c,n,kqfilter), 0 }

/* open, close, read, ioctl */
#define cdev_ipf_init(c,n)	cdev__ocri_init(c,n)

/* open */
#define	cdev_svr4_net_init(c,n) { \
	dev_init(c,n,open), dev_noimpl(close,enodev), \
	dev_noimpl(read,enodev), dev_noimpl(write,enodev), \
	dev_noimpl(ioctl,enodev), dev_noimpl(stop,enodev), \
	0, seltrue, dev_noimpl(mmap,enodev), dev_noimpl(kqfilter,enodev), 0 }

/* open, close, read, write, ioctl, stop, tty, poll, mmap */
#define cdev_wsdisplay_init(c,n)	cdev__ttym_init(c,n,D_TTY)

/* open, close, (read), (write), ioctl, poll, mmap -- XXX should be a map device */
#define	cdev_grf_init(c,n)	cdev__ocRWipm_init(c,n)
#define	cdev_view_init(c,n)	cdev__ocRWipm_init(c,n)

/* open, close, write, ioctl */
#define	cdev_spkr_init(c,n)	cdev__ocwi_init(c,n)

/* open, close, read, write, ioctl, poll */
#define cdev_vc_nb_init(c,n)	cdev__ocrwip_init(c,n)

/* open, close, read, write, ioctl, stop, poll */
#define	cdev_esh_init(c,n)	cdev__ocrwisp_init(c,n)

/* open, close, read, ioctl, mmap */
#define cdev_bktr_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_noimpl(write,enodev), dev_init(c,n,ioctl), \
	dev_noimpl(stop,enodev), 0, seltrue, \
	dev_init(c,n,mmap), dev_noimpl(kqfilter,enodev), 0 }

/* open, close, read, write, ioctl, stop, tty, poll, mmap */
#define	cdev_pc_init(c,n)	cdev__ttym_init(c,n,D_TTY)
#define	pckqfilter		ttykqfilter

/* open, close, write, ioctl */
#define cdev_lpt_init(c,n)	cdev__ocwi_init(c,n)

/* open, close, read, ioctl */
#define	cdev_joy_init(c,n)	cdev__ocri_init(c,n)

/* open, close, ioctl, poll -- XXX should be a generic device */
#define cdev_apm_init(c,n)	cdev__ocip_init(c,n)

/* open, close, read, ioctl, poll */
#define cdev_satlink_init(c,n)	cdev__ocrip_init(c,n)

/* open, close, ioctl */
#define cdev_wdog_init(c,n)	cdev__oci_init(c,n)

/* open, close, ioctl */
#define cdev_i4bctl_init(c,n)	cdev__oci_init(c,n)

/* open, close, read, write, ioctl, poll */
#define	cdev_i4brbch_init(c,n)	cdev__ocrwip_init(c,n)

/* open, close, read, write, poll */
#define	cdev_i4btel_init(c,n)	cdev__ocrwp_init(c,n)

/* open, close, read, ioctl */
#define cdev_i4btrc_init(c,n)	cdev__ocri_init(c,n)

/* open, close, read, ioctl, poll */
#define cdev_i4b_init(c,n)	cdev__ocrip_init(c,n)

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

int	nullioctl __P((struct tty *, u_long, caddr_t, int, struct proc *));
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

dev_t	chrtoblk __P((dev_t));
int	iskmemdev __P((dev_t));
int	iszerodev __P((dev_t));

/*
 * [bc]dev_decl()s for 'fake' tty devices.
 */
cdev_decl(cn);

cdev_decl(ctty);

#define	ptctty		ptytty
#define	ptcioctl	ptyioctl
#define	ptckqfilter	ttykqfilter
cdev_decl(ptc);

#define	ptstty		ptytty
#define	ptsioctl	ptyioctl
#define	ptskqfilter	ttykqfilter
cdev_decl(pts);

/*
 * [bc]dev_decl()s for 'fake' disk devices.
 */
bdev_decl(ccd);
cdev_decl(ccd);

bdev_decl(md);
cdev_decl(md);

bdev_decl(raid);
cdev_decl(raid);

bdev_decl(vnd);
cdev_decl(vnd);

/*
 * [bc]dev_decl()s for SCSI devices.
 */
bdev_decl(cd);
cdev_decl(cd);

cdev_decl(ch);

bdev_decl(sd);
cdev_decl(sd);

/* XXX Namespace collisions with SYSVSEM; just declare what we need. */
dev_decl(se,open); dev_decl(se,close); dev_decl(se,ioctl);

bdev_decl(st);
cdev_decl(st);

bdev_decl(ss);
cdev_decl(ss);

bdev_decl(uk);
cdev_decl(uk);

/*
 * [bc]dev_decl()s for logical disks.
 */
bdev_decl(ld);
cdev_decl(ld);

/*
 * cdev_decl()s for Brooktree 8[47][89] based TV cards.
 */
cdev_decl(bktr);

/*
 * [bc]dev_decl()s for 'fake' network devices.
 */
cdev_decl(bpf);

cdev_decl(ipl);

#ifdef COMPAT_SVR4
# define NSVR4_NET	1
#else
# define NSVR4_NET	0
#endif
cdev_decl(svr4_net);

cdev_decl(tun);

/*
 * [bc]dev_decl()s for miscellaneous 'fake' devices. 
 */
cdev_decl(audio);
cdev_decl(midi);
cdev_decl(sequencer);

cdev_decl(filedesc);

#ifndef LKM
# define	NLKM	0
#else
# define	NLKM	1
#endif
cdev_decl(lkm);

cdev_decl(log);

cdev_decl(rnd);

#endif /* _KERNEL */

/*
 * Used by setroot() to map device names to bdev major numbers.
 * Ports declare a NULL-terminated array of these structures
 * to setroot().
 */
struct devnametobdevmaj {
	const char *d_name;
	int	d_maj;
};

#ifdef _KERNEL
extern	struct devnametobdevmaj dev_name2blk[];
struct	device;
void	setroot __P((struct device *, int));
void	swapconf __P((void));
#endif /* _KERNEL */

#endif /* !_SYS_CONF_H_ */
