/*	$NetBSD: mem.c,v 1.26.6.7 2010/11/06 08:08:14 uebayasi Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1990, 1993
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
 */
/*
 * Copyright (c) 1988 University of Utah.
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
 */

/*
 * Memory special file
 */

#include "opt_arm32_pmap.h"
#include "opt_compat_netbsd.h"
#include "opt_xip.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mem.c,v 1.26.6.7 2010/11/06 08:08:14 uebayasi Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
#include <sys/kauth.h>

#include <machine/cpu.h>

#include <uvm/uvm.h>

#define	VM_PAGE_TO_MD(pg)	(&(pg)->mdpage)

extern vaddr_t memhook;			/* in pmap.c (poor name!) */
extern kmutex_t memlock;		/* in pmap.c */
extern void *zeropage;			/* in pmap.c */

dev_type_read(mmrw);
dev_type_ioctl(mmioctl);
dev_type_mmap(mmmmap);

const struct cdevsw mem_cdevsw = {
	.d_open = nullopen,
	.d_close = nullclose, 
	.d_read = mmrw,
	.d_write = mmrw,
	.d_ioctl = mmioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = mmmmap,
	.d_kqfilter = nokqfilter,
	.d_flag = D_MPSAFE
};

/*ARGSUSED*/
int
mmrw(dev_t dev, struct uio *uio, int flags)
{
	vaddr_t o, v, m;
	int c;
	struct iovec *iov;
	int error = 0;
	vm_prot_t prot;

	while (uio->uio_resid > 0 && error == 0) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("mmrw");
			continue;
		}
		switch (minor(dev)) {

		case DEV_MEM:
			v = uio->uio_offset;
			prot = uio->uio_rw == UIO_READ ? VM_PROT_READ :
			    VM_PROT_WRITE;
			m = memhook;
#ifdef PMAP_CACHE_VIPT
			{
				struct vm_page *pg;
				pg = PHYS_TO_VM_PAGE(trunc_page(v));
				if (pg != NULL && pmap_is_page_colored_p(VM_PAGE_TO_MD(pg)))
					o = VM_PAGE_TO_MD(pg)->pvh_attrs;
				else
					o = v;
				m += o & arm_cache_prefer_mask;
			}
#endif
			mutex_enter(&memlock);
			pmap_enter(pmap_kernel(), m,
			    trunc_page(v), prot, prot|PMAP_WIRED);
			pmap_update(pmap_kernel());
			o = uio->uio_offset & PGOFSET;
			c = min(uio->uio_resid, (int)(PAGE_SIZE - o));
			error = uiomove((char *)m + o, c, uio);
			pmap_remove(pmap_kernel(), m, m + PAGE_SIZE);
			pmap_update(pmap_kernel());
			mutex_exit(&memlock);
			break;

		case DEV_KMEM:
			v = uio->uio_offset;
			c = min(iov->iov_len, MAXPHYS);
			if (!uvm_kernacc((void *)v, c,
			    uio->uio_rw == UIO_READ ? B_READ : B_WRITE))
				return (EFAULT);
			error = uiomove((void *)v, c, uio);
			break;

		case DEV_NULL:
			if (uio->uio_rw == UIO_WRITE)
				uio->uio_resid = 0;
			return (0);

#ifdef COMPAT_16
		case _DEV_ZERO_oARM:
#endif
		case DEV_ZERO:
			if (uio->uio_rw == UIO_WRITE) {
				uio->uio_resid = 0;
				return (0);
			}
			c = min(iov->iov_len, PAGE_SIZE);
			error = uiomove(zeropage, c, uio);
			break;

		default:
			return (ENXIO);
		}
	}
	return (error);
}

paddr_t
mmmmap(dev_t dev, off_t off, int prot)
{
	struct lwp *l = curlwp;	/* XXX */

	/*
	 * /dev/mem is the only one that makes sense through this
	 * interface.  For /dev/kmem any physaddr we return here
	 * could be transient and hence incorrect or invalid at
	 * a later time.  /dev/null just doesn't make any sense
	 * and /dev/zero is a hack that is handled via the default
	 * pager in mmap().
	 */
	if (minor(dev) != DEV_MEM)
		return (-1);

	/* minor device 0 is physical memory */

	if (off >= ctob(physmem) && kauth_authorize_machdep(l->l_cred,
	    KAUTH_MACHDEP_UNMANAGEDMEM, NULL, NULL, NULL, NULL) != 0)
		return -1;

	return pmap_mmap(0, off);
}
