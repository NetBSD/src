/*	$NetBSD: vndvar.h,v 1.5 2000/01/21 23:39:57 thorpej Exp $	*/

/*-     
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors. 
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

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: fdioctl.h 1.1 90/07/09$
 *
 *	@(#)vnioctl.h	8.1 (Berkeley) 6/10/93
 */

#include <sys/pool.h>

/*
 * Vnode disk pseudo-geometry information.
 */
struct vndgeom {
	u_int32_t	vng_secsize;	/* # bytes per sector */
	u_int32_t	vng_nsectors;	/* # data sectors per track */
	u_int32_t	vng_ntracks;	/* # tracks per cylinder */
	u_int32_t	vng_ncylinders;	/* # cylinders per unit */
};

/*
 * Ioctl definitions for file (vnode) disk pseudo-device.
 */
struct vnd_ioctl {
	char		*vnd_file;	/* pathname of file to mount */
	int		vnd_flags;	/* flags; see below */
	struct vndgeom	vnd_geom;	/* geometry to emulate */
	int		vnd_size;	/* (returned) size of disk */
};

/* vnd_flags */
#define	VNDIOF_HASGEOM	0x01		/* use specified geometry */

struct vnode;
struct ucred;

/*
 * A vnode disk's state information.
 */
struct vnd_softc {
	int		 sc_unit;	/* logical unit number */
	int		 sc_flags;	/* flags */
	size_t		 sc_size;	/* size of vnd */
	struct vnode	*sc_vp;		/* vnode */
	struct ucred	*sc_cred;	/* credentials */
	int		 sc_maxactive;	/* max # of active requests */
	struct buf_queue sc_tab;	/* transfer queue */
	int		 sc_active;	/* number of active transfers */
	char		 sc_xname[8];	/* XXX external name */
	struct disk	 sc_dkdev;	/* generic disk device info */
	struct vndgeom	 sc_geom;	/* virtual geometry */
	struct pool	 sc_vxpool;	/* vndxfer pool */
	struct pool	 sc_vbpool;	/* vndbuf pool */
};

/* sc_flags */
#define	VNF_INITED	0x01	/* unit has been initialized */
#define	VNF_WLABEL	0x02	/* label area is writable */
#define	VNF_LABELLING	0x04	/* unit is currently being labelled */
#define	VNF_WANTED	0x08	/* someone is waiting to obtain a lock */
#define	VNF_LOCKED	0x10	/* unit is locked */
#define	VNF_BUSY	0x20	/* unit is busy */

/*
 * Before you can use a unit, it must be configured with VNDIOCSET.
 * The configuration persists across opens and closes of the device;
 * an VNDIOCCLR must be used to reset a configuration.  An attempt to
 * VNDIOCSET an already active unit will return EBUSY.
 */
#define VNDIOCSET	_IOWR('F', 0, struct vnd_ioctl)	/* enable disk */
#define VNDIOCCLR	_IOW('F', 1, struct vnd_ioctl)	/* disable disk */
