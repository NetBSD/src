/*	$NetBSD: mem.c,v 1.5 1999/02/10 17:03:27 kleink Exp $ */

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
 *	@(#)mem.c	8.3 (Berkeley) 1/12/94
 */

/*
 * Memory special file
 */

#include "opt_uvm.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/conf.h>

#include <sparc64/sparc64/vaddrs.h>
#include <machine/eeprom.h>
#include <machine/conf.h>
#include <machine/ctlreg.h>

#include <vm/vm.h>

extern vaddr_t prom_vstart;
extern vaddr_t prom_vend;
caddr_t zeropage;

/*ARGSUSED*/
int
mmopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{

	return (0);
}

/*ARGSUSED*/
int
mmclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{

	return (0);
}

/*ARGSUSED*/
int
mmrw(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	register vaddr_t o, v;
	register int c;
	register struct iovec *iov;
	int error = 0;
	static int physlock;
	extern caddr_t vmmap;

	if (minor(dev) == 0) {
		/* lock against other uses of shared vmmap */
		while (physlock > 0) {
			physlock++;
			error = tsleep((caddr_t)&physlock, PZERO | PCATCH,
			    "mmrw", 0);
			if (error)
				return (error);
		}
		physlock = 1;
	}
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

		/* minor device 0 is physical memory */
		case 0:
#if 1
			v = uio->uio_offset;
			if (!pmap_pa_exists(v)) {
				error = EFAULT;
				goto unlock;
			}
			pmap_enter(pmap_kernel(), (vaddr_t)vmmap,
			    trunc_page(v), uio->uio_rw == UIO_READ ?
			    VM_PROT_READ : VM_PROT_WRITE, TRUE);
			o = uio->uio_offset & PGOFSET;
			c = min(uio->uio_resid, (int)(NBPG - o));
			error = uiomove((caddr_t)vmmap + o, c, uio);
			pmap_remove(pmap_kernel(), (vaddr_t)vmmap,
			    (vaddr_t)vmmap + NBPG);
			break;
#else
			/* On v9 we can just use the physical ASI and not bother w/mapin & mapout */
			v = uio->uio_offset;
			if (!pmap_pa_exists(v)) {
				error = EFAULT;
				goto unlock;
			}
			o = uio->uio_offset & PGOFSET;
			c = min(uio->uio_resid, (int)(NBPG - o));
			/* However, we do need to partially re-implement uiomove() */
			if (uio->uio_rw != UIO_READ && uio->uio_rw != UIO_WRITE)
				panic("mmrw: uio mode");
			if (uio->uio_segflg == UIO_USERSPACE && uio->uio_procp != curproc)
				panic("mmrw: uio proc");
			while (c > 0 && uio->uio_resid) {
				register struct iovec *iov;
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
				iov->iov_base =  (caddr_t)iov->iov_base + cnt;
				iov->iov_len -= cnt;
				uio->uio_resid -= cnt;
				uio->uio_offset += cnt;
				c -= cnt;
			}
			/* Should not be necessary */
			blast_vcache();
			break;
#endif

		/* minor device 1 is kernel memory */
		case 1:
			v = uio->uio_offset;
			if (v >= MSGBUF_VA && v < MSGBUF_VA+NBPG) {
				c = min(iov->iov_len, 4096);
#if 0		/* Don't know where PROMs are on Ultras.  Think it's at f000000 */
			} else if (v >= prom_vstart && v < prom_vend &&
				   uio->uio_rw == UIO_READ) {
				/* Allow read-only access to the PROM */
				c = min(iov->iov_len, prom_vend - prom_vstart);
#endif
			} else {
				c = min(iov->iov_len, MAXPHYS);
#if defined(UVM)
				if (!uvm_kernacc((caddr_t)v, c,
				    uio->uio_rw == UIO_READ ? B_READ : B_WRITE))
					return (EFAULT);
#else
				if (!kernacc((caddr_t)v, c,
				    uio->uio_rw == UIO_READ ? B_READ : B_WRITE))
					return (EFAULT);
#endif
			}
			error = uiomove((caddr_t)v, c, uio);
			break;

		/* minor device 2 is EOF/RATHOLE */
		case 2:
			if (uio->uio_rw == UIO_WRITE)
				uio->uio_resid = 0;
			return (0);

/* XXX should add sbus, etc */

#if defined(SUN4)
		/*
		 * minor device 11 (/dev/eeprom) is the old-style
		 * (a'la Sun 3) EEPROM.
		 */
		case 11:
			if (cputyp == CPU_SUN4)
				error = eeprom_uio(uio);
			else
				error = ENXIO;

			break;
#endif /* SUN4 */

		/*
		 * minor device 12 (/dev/zero) is source of nulls on read,
		 * rathole on write.
		 */
		case 12:
			if (uio->uio_rw == UIO_WRITE) {
				uio->uio_resid = 0;
				return(0);
			}
			if (zeropage == NULL) {
				zeropage = (caddr_t)
				    malloc(CLBYTES, M_TEMP, M_WAITOK);
				bzero(zeropage, CLBYTES);
			}
			c = min(iov->iov_len, CLBYTES);
			error = uiomove(zeropage, c, uio);
			break;

		default:
			return (ENXIO);
		}
	}
	if (minor(dev) == 0) {
unlock:
		if (physlock > 1)
			wakeup((caddr_t)&physlock);
		physlock = 0;
	}
	return (error);
}

int
mmmmap(dev, off, prot)
        dev_t dev;
        int off, prot;
{

	return (-1);
}
