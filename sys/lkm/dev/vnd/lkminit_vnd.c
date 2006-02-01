/*	$NetBSD: lkminit_vnd.c,v 1.7 2006/02/01 20:55:31 martin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: lkminit_vnd.c,v 1.7 2006/02/01 20:55:31 martin Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/lkm.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/device.h>

CFDRIVER_DECL(vnd, DV_DISK, NULL);

extern struct cfattach vnd_ca;
extern const struct bdevsw vnd_bdevsw;
extern const struct cdevsw vnd_cdevsw;
extern void vndattach (int);
extern int vnd_destroy (struct device *);

static int vnd_lkm(struct lkm_table *lkmtp, int cmd);
int vnd_lkmentry(struct lkm_table *lkmtp, int cmd, int ver);

MOD_DEV("vnd", "vnd", &vnd_bdevsw, -1, &vnd_cdevsw, -1);

static int
vnd_lkm(struct lkm_table *lkmtp, int cmd)
{
	int error = 0, i;

	if (cmd == LKM_E_LOAD) {
		error = config_cfdriver_attach(&vnd_cd);
		if (error) {
			aprint_error("%s: unable to register cfdriver\n",
			    vnd_cd.cd_name);
			return error;
		}
		
		vndattach(0);
	} else if (cmd == LKM_E_UNLOAD) {
		for (i = 0; i < vnd_cd.cd_ndevs; i++)
			if (vnd_cd.cd_devs[i] != NULL &&
			    (error = vnd_destroy(vnd_cd.cd_devs[i])) != 0)
				return 0;

		if ((error = config_cfattach_detach(vnd_cd.cd_name,
		    &vnd_ca)) != 0) {
			aprint_error("%s: unable to deregister cfattach\n",
			    vnd_cd.cd_name);
			return error;
		}

		if ((error = config_cfdriver_detach(&vnd_cd)) != 0) {
			aprint_error("%s: unable to deregister cfdriver\n",
			    vnd_cd.cd_name);
			return error;
		}
	}
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
