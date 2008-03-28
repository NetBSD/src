/*	$NetBSD: lkminit_test.c,v 1.2 2008/03/28 20:28:27 ad Exp $	*/

/*
 * Copyright (c) 1993 Terrence R. Lambert.
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
 *      This product includes software developed by Terrence R. Lambert.
 * 4. The name Terrence R. Lambert may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TERRENCE R. LAMBERT ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE TERRENCE R. LAMBERT BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from NetBSD: lkminit_test.c,v 1.6 2005/12/11 12:24:49 christos Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lkminit_test.c,v 1.2 2008/03/28 20:28:27 ad Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/lkm.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/syscall.h>

int	testcall(struct lwp *, void *, register_t *);
int	LKMENTRY(struct lkm_table *lkmtp, int, int);
int	sys_lkmnosys(struct lwp *, const void *, register_t *);
int	testmod_handle(struct lkm_table *, int);

static struct sysent newent = {
	0, 0, 0, (void *)testcall
};

static struct sysent	oldent;

MOD_MISC("testmod");

int
testmod_handle(struct lkm_table *lkmtp, int cmd)
{
	struct lkm_misc *args = lkmtp->private.lkm_misc;
	int i, err = 0;

	switch( cmd) {
	case LKM_E_LOAD:
		if(lkmexists( lkmtp))
			return( EEXIST);
		for (i = 0; i < SYS_MAXSYSCALL; i++)
			if ((void *)sysent[i].sy_call == sys_lkmnosys)
				break;
		if (i == SYS_MAXSYSCALL) {
			err = ENFILE;
			break;
		}
		memcpy(&oldent, &sysent[i], sizeof(struct sysent));
		memcpy(&sysent[i], &newent, sizeof(struct sysent));
		args->mod.lkm_offset = i;
		break;

	case LKM_E_UNLOAD:
		i = args->mod.lkm_offset;
		memcpy(&sysent[i], &oldent, sizeof(struct sysent));
		break;

	default:
		err = EINVAL;
		break;
	}

	return err;
}

int
LKMENTRY(struct lkm_table *lkmtp, int cmd, int ver)
{
	DISPATCH(lkmtp,cmd,ver,testmod_handle,testmod_handle,lkm_nofunc)
}
