/*	$NetBSD: iwm_mod.c,v 1.5.8.2 2002/05/19 14:15:09 gehenna Exp $	*/

/*
 * Copyright (c) 1997, 1998 Hauke Fath.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 * Sony (floppy disk) driver for Macintosh m68k, module entry.
 * This is derived from Terry Lambert's LKM examples.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iwm_mod.c,v 1.5.8.2 2002/05/19 14:15:09 gehenna Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/lkm.h>
#include <sys/file.h>
#include <sys/errno.h>

/* The module entry */
int iwmfd_lkmentry __P((struct lkm_table *lkmtp, int cmd, int ver));

extern int fd_mod_init __P((void));
extern void fd_mod_free __P((void));

extern const struct bdevsw fd_bdevsw;
extern const struct cdevsw fd_cdevsw;

MOD_DEV("iwmfd", "fd", &fd_bdevsw, -1, &fd_cdevsw, -1)

/*
 * iwmfd_lkmentry
 *
 * External entry point; should generally match name of .o file.
 */
int
iwmfd_lkmentry (lkmtp, cmd, ver)
	struct lkm_table *lkmtp;		  
	int cmd;
	int ver;
{
	int error = 0;

	if (ver != LKM_VERSION)
		return (EINVAL);

	switch (cmd) {
	case LKM_E_LOAD:
		lkmtp->private.lkm_any = (struct lkm_any *)&_module;
		error = lkmdispatch(lkmtp, cmd);
		if (error != 0)
			break;
		error = fd_mod_init();
		break;

	case LKM_E_UNLOAD:
		fd_mod_free();
		lkmtp->private.lkm_any = (struct lkm_any *)&_module;
		error = lkmdispatch(lkmtp, cmd);
		break;

	case LKM_E_STAT:
		error = lkmdispatch(lkmtp, cmd);
		break;
	}

	return (error);
}
