/*	$NetBSD: lkminit_vnd.c,v 1.2 2004/03/21 10:51:16 mrg Exp $	*/

/*
 * Copyright (c) 2002 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lkminit_vnd.c,v 1.2 2004/03/21 10:51:16 mrg Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/lkm.h>
#include <sys/file.h>
#include <sys/errno.h>

extern const struct bdevsw vnd_bdevsw;
extern const struct cdevsw vnd_cdevsw;
extern void vndattach (int);
extern int vnddetach (void);

static int vnd_lkm(struct lkm_table *lkmtp, int cmd);
int vnd_lkmentry(struct lkm_table *lkmtp, int cmd, int ver);

MOD_DEV("vnd", "vnd", &vnd_bdevsw, -1, &vnd_cdevsw, -1);

/*
 * Randomly choose 4; should have vnconfig (un)create each vnd ala "gif"..
 */
#define NVNDLKM 4
int num_vnd_lkm = NVNDLKM;

static int
vnd_lkm(struct lkm_table *lkmtp, int cmd)
{
	int error = 0;

	if (cmd == LKM_E_LOAD)
		vndattach(num_vnd_lkm);
	else if (cmd == LKM_E_UNLOAD)
		error = vnddetach();
	return (error);
}

int
vnd_lkmentry (lkmtp, cmd, ver)
	struct lkm_table *lkmtp;		  
	int cmd;
	int ver;
{

	DISPATCH(lkmtp, cmd, ver, vnd_lkm, vnd_lkm, lkm_nofunc);
}
