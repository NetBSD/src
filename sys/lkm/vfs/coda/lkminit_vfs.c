/* $NetBSD: lkminit_vfs.c,v 1.2 2000/03/21 11:45:58 simonb Exp $ */

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael Graff <explorer@flame.org>.
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

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/lkm.h>
#include <sys/file.h>
#include <sys/errno.h>

#include <coda/coda_psdev.h>

int coda_lkmentry __P((struct lkm_table *, int, int));

/*
 * This is the vfsops table for the file system in question
 */
extern struct vfsops coda_vfsops;

/*
 * declare the filesystem
 */
/*
MOD_VFS("coda", -1, &coda_vfsops);
*/
struct lkm_vfs coda_lkm_vfs = {
	LM_VFS, LKM_VERSION, "coda", -1, (void *)&coda_vfsops};



/*
 * declare up/down call device
 */
struct cdevsw codadevsw = {
	vc_nb_open,		/* open */
	vc_nb_close,		/* close */
	vc_nb_read,		/* read */
	vc_nb_write,		/* write */
	vc_nb_ioctl,		/* ioctl */
	0,			/* stop */
	0,			/* tty */
	vc_nb_poll,		/* poll */
	0,			/* mmap */
};

/*
MOD_DEV("codadev", LM_DT_CHAR, 60, &codadevsw);
*/
struct lkm_dev coda_lkm_dev = {
	LM_DEV, LKM_VERSION, "codadev", 60, LM_DT_CHAR,
	{ (void *) &codadevsw }
};


/*
 * entry point
 */
int
coda_lkmentry(lkmtp, cmd, ver)
	struct lkm_table *lkmtp;	
	int cmd;
	int ver;
{
	int error = 0;

	if (ver != LKM_VERSION)
		return EINVAL;

	switch (cmd) {
	case LKM_E_LOAD:
		lkmtp->private.lkm_any = (struct lkm_any *) &coda_lkm_dev;
		error = lkmdispatch(lkmtp, cmd);
		if (error)
			break;
		lkmtp->private.lkm_any = (struct lkm_any *) &coda_lkm_vfs ;
		error = lkmdispatch(lkmtp, cmd);
		break;
	case LKM_E_UNLOAD:
		lkmtp->private.lkm_any = (struct lkm_any *) &coda_lkm_vfs ;
		error = lkmdispatch(lkmtp, cmd);
		if (error)
			break;
		lkmtp->private.lkm_any = (struct lkm_any *) &coda_lkm_dev;
		error = lkmdispatch(lkmtp, cmd);
		break;
	case LKM_E_STAT:
		error = lkmdispatch(lkmtp, cmd);
		break;
	}
	return error;

}
