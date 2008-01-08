/* $NetBSD: cgdvar.h,v 1.9.28.1 2008/01/08 22:10:54 bouyer Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland C. Dowdeswell.
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

#ifndef _DEV_CGDVAR_H_
#define	_DEV_CGDVAR_H_

#include <sys/simplelock.h>

/* ioctl(2) code */
struct cgd_ioctl {
	const char	*ci_disk;
	int		 ci_flags;
	int		 ci_unit;
	size_t		 ci_size;
	const char	*ci_alg;
	const char	*ci_ivmethod;
	size_t		 ci_keylen;
	const char	*ci_key;
	size_t		 ci_blocksize;
};

#ifdef _KERNEL

#include <dev/cgd_crypto.h>

/* This cryptdata structure is here rather than cgd_crypto.h, since
 * it stores local state which will not be generalised beyond the
 * cgd driver.
 */

struct cryptdata {
	size_t		 cf_blocksize;	/* block size (in bytes) */
	int		 cf_mode;	/* Cipher Mode and IV Gen method */
#define CGD_CIPHER_CBC_ENCBLKNO 1	/* CBC Mode w/ Enc Block Number */
	void		*cf_priv;	/* enc alg private data */
};

struct cgd_softc {
	struct dk_softc		 sc_dksc;	/* generic disk interface */
	struct cryptinfo	*sc_crypt;	/* the alg/key/etc */
	struct vnode		*sc_tvn;	/* target device's vnode */
	dev_t			 sc_tdev;	/* target device */
	char			*sc_tpath;	/* target device's path */
	void *			 sc_data;	/* emergency buffer */
	int			 sc_data_used;	/* Really lame, we'll change */
	size_t			 sc_tpathlen;	/* length of prior string */
	struct cryptdata	 sc_cdata;	/* crypto data */
	struct cryptfuncs	*sc_cfuncs;	/* encryption functions */
	struct simplelock	 sc_slock;	/* our lock */
};
#endif

/* XXX XAX XXX elric:  check these out properly. */
#define CGDIOCSET	_IOWR('F', 18, struct cgd_ioctl)
#define CGDIOCCLR	_IOW('F', 19, struct cgd_ioctl)

#endif /* _DEV_CGDVAR_H_ */
