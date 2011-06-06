/*	$NetBSD: mem.c,v 1.35.6.1 2011/06/06 09:06:53 jruoho Exp $ */

/*
 * Copyright (c) 1988 University of Utah.
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
 *
 *	@(#)mem.c	8.3 (Berkeley) 1/12/94
 */

/*
 * Memory special file
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mem.c,v 1.35.6.1 2011/06/06 09:06:53 jruoho Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/msgbuf.h>

#include <machine/eeprom.h>
#include <machine/ctlreg.h>

#include <uvm/uvm_extern.h>

vaddr_t prom_vstart = 0xf000000;
vaddr_t prom_vend = 0xf0100000;
void *zeropage;

dev_type_read(mmrw);
dev_type_ioctl(mmioctl);

const struct cdevsw mem_cdevsw = {
	nullopen, nullclose, mmrw, mmrw, mmioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_OTHER
};

/*ARGSUSED*/
int
mmrw(dev_t dev, struct uio *uio, int flags)
{
	vaddr_t o, v;
	int c;
	struct iovec *iov;
	int error = 0;
	static int physlock;
	vm_prot_t prot;
	extern void *vmmap;
	vsize_t msgbufsz;

	if (minor(dev) == DEV_MEM) {
		/* lock against other uses of shared vmmap */
		while (physlock > 0) {
			physlock++;
			error = tsleep((void *)&physlock, PZERO | PCATCH,
			    "mmrw", 0);
			if (error)
				return (error);
		}
		physlock = 1;
	}
	while (uio->uio_resid > 0 && error == 0) {
		int n;
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("mmrw");
			continue;
		}

		/* Note how much is still to go */
		n = uio->uio_resid;

		switch (minor(dev)) {

		case DEV_MEM:
#if 1
			v = uio->uio_offset;
			if (!pmap_pa_exists(v)) {
				error = EFAULT;
				goto unlock;
			}
			prot = uio->uio_rw == UIO_READ ? VM_PROT_READ :
			    VM_PROT_WRITE;
			pmap_enter(pmap_kernel(), (vaddr_t)vmmap,
			    trunc_page(v), prot, prot|PMAP_WIRED);
			pmap_update(pmap_kernel());
			o = uio->uio_offset & PGOFSET;
			c = min(uio->uio_resid, (int)(PAGE_SIZE - o));
			error = uiomove((char *)vmmap + o, c, uio);
			pmap_remove(pmap_kernel(), (vaddr_t)vmmap,
			    (vaddr_t)vmmap + PAGE_SIZE);
			pmap_update(pmap_kernel());
			break;
#else
			/* On v9 we can just use the physical ASI and not bother w/mapin & mapout */
			v = uio->uio_offset;
			if (!pmap_pa_exists(v)) {
				error = EFAULT;
				goto unlock;
			}
			o = uio->uio_offset & PGOFSET;
			c = min(uio->uio_resid, (int)(PAGE_SIZE - o));
			/* However, we do need to partially re-implement uiomove() */
			if (uio->uio_rw != UIO_READ && uio->uio_rw != UIO_WRITE)
				panic("mmrw: uio mode");
			if (uio->uio_segflg == UIO_USERSPACE && uio->uio_lwp != curlwp)
				panic("mmrw: uio proc");
			while (c > 0 && uio->uio_resid) {
				struct iovec *iov;
				u_int cnt;
				int d;

				iov = uio->uio_iov;
				cnt = iov->iov_len;
				if (cnt == 0) {
					uio->uio_iov++;
					uio->uio_iovcnt--;
					continue;
				}
				if (cnt > c)
					cnt = c;
				d = iov->iov_base;
				switch (uio->uio_segflg) {
					
				case UIO_USERSPACE:
					if (uio->uio_rw == UIO_READ)
						while (cnt--)
							if(subyte(d++, lduba(v++, ASI_PHYS_CACHED))) {
								error = EFAULT;
								goto unlock;
							}
					else
						while (cnt--)
							stba(v++, ASI_PHYS_CACHED, fubyte(d++));
					if (error)
						goto unlock;
					break;
					
				case UIO_SYSSPACE:
					if (uio->uio_rw == UIO_READ)
						while (cnt--)
							stba(d++, ASI_P, lduba(v++, ASI_PHYS_CACHED));
					else
						while (cnt--)
							stba(v++, ASI_PHYS_CACHED, lduba(d++, ASI_P));
					break;
				}
				iov->iov_base =  (void *)iov->iov_base + cnt;
				iov->iov_len -= cnt;
				uio->uio_resid -= cnt;
				uio->uio_offset += cnt;
				c -= cnt;
			}
			break;
#endif

		case DEV_KMEM:
			v = uio->uio_offset;
			msgbufsz = msgbufp->msg_bufs +
				offsetof(struct kern_msgbuf, msg_bufc);
			if (v >= (vaddr_t)msgbufp &&
			    v < (vaddr_t)(msgbufp + msgbufsz)) {
				c = min(iov->iov_len, msgbufsz);
#if 1		/* Don't know where PROMs are on Ultras.  Think it's at f000000 */
			} else if (v >= prom_vstart && v < prom_vend &&
				   uio->uio_rw == UIO_READ) {
				/* Allow read-only access to the PROM */
				c = min(iov->iov_len, prom_vend - prom_vstart);
#endif
			} else {
				c = min(iov->iov_len, MAXPHYS);
				if (!uvm_kernacc((void *)v, c,
				    uio->uio_rw == UIO_READ ? B_READ : B_WRITE))
					return (EFAULT);
			}
			error = uiomove((void *)v, c, uio);
			break;

		case DEV_NULL:
			if (uio->uio_rw == UIO_WRITE)
				uio->uio_resid = 0;
			return (0);

/* XXX should add sbus, etc */

		case DEV_ZERO:
			if (uio->uio_rw == UIO_WRITE) {
				uio->uio_resid = 0;
				return(0);
			}
			if (zeropage == NULL) {
				zeropage = (void *)
				    malloc(PAGE_SIZE, M_TEMP, M_WAITOK);
				memset(zeropage, 0, PAGE_SIZE);
			}
			c = min(iov->iov_len, PAGE_SIZE);
			error = uiomove(zeropage, c, uio);
			break;

		default:
			return (ENXIO);
		}

		/* If we didn't make any progress (i.e. EOF), we're done here */
		if (n == uio->uio_resid)
			break;
	}
	if (minor(dev) == 0) {
unlock:
		if (physlock > 1)
			wakeup((void *)&physlock);
		physlock = 0;
	}
	return (error);
}
