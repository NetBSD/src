/* $NetBSD: kbio.h,v 1.4.142.1 2015/09/22 12:05:35 skrll Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)kbio.h	8.1 (Berkeley) 6/11/93
 */

#include <sys/ioccom.h>

#if 0 /* XXX */
/*
 * The following is a minimal emulation of Sun's `kio' structures
 * and related operations necessary to make X11 happy (i.e., make it
 * compile, and make old X11 binaries run).
 */

/*
 * The kiockey structure apparently gets and/or sets keyboard mappings.
 * It seems to be kind of useless, but X11 uses it (according to the
 * comments) to figure out when a Sun 386i has a type-4 keyboard but
 * claims to have a type-3 keyboard.  We need just enough to cause the
 * appropriate ioctl to return the appropriate magic value.
 *
 * KIOCGETKEY fills in kio_entry from kio_station.  Not sure what tablemask
 * is for; X sets it before the call, so it is not an output, but we do not
 * care anyway.  KIOCSDIRECT is supposed to tell the kernel whether to send
 * keys to the console or to X; we just send them to X whenever the keyboard
 * is open at all.  (XXX may need to change this later)
 *
 * Keyboard commands and types are defined in kbd.h as they are actually
 * real hardware commands and type numbers.
 */
struct okiockey { /* Out-dated key translation structure */
	int	kio_tablemask;	/* whatever */
	u_char	kio_station;	/* key number */
	u_char	kio_entry;	/* HOLE if not present */
	char	kio_text[10];	/* the silly escape sequences (unsupported) */
};

struct kiockey {
	int	kio_tablemask;	/* whatever */
	u_char	kio_station;	/* key number */
	u_short	kio_entry;	/* HOLE if not present */
	char	kio_text[10];	/* the silly escape sequences (unsupported) */
};

/*
 * Values for kio_tablemask. These determine which table to read/modify
 * in KIOC[SG]KEY ioctls. Currently, we only have "non-shift" and "shift"
 * tables.
 */
#define KIOC_NOMASK	0x0
#define KIOC_CAPSMASK	0x1
#define KIOC_SHIFTMASK	0xe
#define KIOC_CTRLMASK	0x30
#define KIOC_ALTGMASK	0x200
#define KIOC_NUMLMASK	0x800

#define	HOLE	0x302		/* value for kio_entry to say `really type 3' */

#define	KIOCTRANS	_IOW('k', 0, int)	/* set translation mode */
			/* (we only accept TR_UNTRANS_EVENT) */
#define	KIOCGETKEY	_IOWR('k', 2, struct okiockey) /* fill in kio_entry */
#define	KIOCGTRANS	_IOR('k', 5, int)	/* get translation mode */
#define	KIOCCMD		_IOW('k', 8, int)	/* X uses this to ring bell */
#define	KIOCTYPE	_IOR('k', 9, int)	/* get keyboard type */
#endif /* 0 XXX */
#define	KIOCSDIRECT	_IOW('k', 10, int)	/* keys to console? */
#if 0 /* XXX */
#define KIOCSKEY	_IOW('k', 12, struct kiockey) /* set xlat mode */
#define KIOCGKEY	_IOWR('k', 13, struct kiockey) /* get xlat mode */
#define	KIOCLAYOUT	_IOR('k', 20, int)	/* get keyboard layout */
#define	KIOCSLED	_IOW('k', 14, char)	/* set LED state */
#define	KIOCGLED	_IOR('k', 15, char)	/* get LED state */

#define	TR_NONE			0	/* X compat, unsupported */
#define	TR_ASCII		1	/* X compat, unsupported */
#define	TR_EVENT		2	/* X compat, unsupported */
#define	TR_UNTRANS_EVENT	3
#endif /* 0 XXX */
