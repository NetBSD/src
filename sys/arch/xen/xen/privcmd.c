/* $NetBSD: privcmd.c,v 1.1 2004/05/07 15:51:04 cl Exp $ */

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
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


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: privcmd.c,v 1.1 2004/05/07 15:51:04 cl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/dirent.h>
#include <sys/stat.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/kernfs/kernfs.h>

#include <machine/kernfs_machdep.h>
#include <machine/xenio.h>

#define	PRIVCMD_MODE	(S_IRUSR)


static int
privcmd_ioctl(void *v)
{
	struct vop_ioctl_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		u_long a_command;
		void *a_data;
		int a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	int error = 0;

	switch (ap->a_command) {
	case IOCTL_PRIVCMD_HYPERCALL:
		__asm__ __volatile__ (
			"pushl %%ebx; pushl %%ecx; pushl %%edx;"
			"pushl %%esi; pushl %%edi; "
			"movl  4(%%eax),%%ebx ;"
			"movl  8(%%eax),%%ecx ;"
			"movl 12(%%eax),%%edx ;"
			"movl 16(%%eax),%%esi ;"
			"movl 20(%%eax),%%edi ;"
			"movl   (%%eax),%%eax ;"
			TRAP_INSTR "; "
			"popl %%edi; popl %%esi; popl %%edx;"
			"popl %%ecx; popl %%ebx"
			: "=a" (error) : "0" (ap->a_data) : "memory" );
		error = -error;
		break;
	}

	return error;
}

static const struct kernfs_fileop privcmd_fileops[] = {
  { .kf_fileop = KERNFS_FILEOP_IOCTL, .kf_vop = privcmd_ioctl },
};

void
xenprivcmd_init()
{
	kernfs_entry_t *dkt;
	kfstype kfst;

	if ((xen_start_info.flags & SIF_PRIVILEGED) == 0)
		return;

	kfst = KERNFS_ALLOCTYPE(privcmd_fileops);

	KERNFS_ALLOCENTRY(dkt, M_TEMP, M_WAITOK);
	KERNFS_INITENTRY(dkt, DT_REG, "privcmd", NULL, kfst, VREG,
	    PRIVCMD_MODE);
	kernfs_addentry(kernxen_pkt, dkt);
}
