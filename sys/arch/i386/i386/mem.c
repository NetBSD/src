/*	$NetBSD: mem.c,v 1.25 1995/04/10 01:57:50 mycroft Exp $	*/

/*-
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, and code derived from software contributed to
 * Berkeley by William Jolitz.
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
 * from: Utah Hdr: mem.c 1.13 89/10/08
 *
 *	@(#)mem.c 7.2 (Berkeley) 5/9/91
 */

/*
 * Memory special file
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/fcntl.h>

#include <machine/cpu.h>

#include <vm/vm.h>

extern char *vmmap;            /* poor name! */
caddr_t zeropage;

/*ARGSUSED*/
int
mmopen(dev, flag, mode)
	dev_t dev;
	int flag, mode;
{

	switch (minor(dev)) {
	case 14:
		if (flag & FWRITE) {
			struct trapframe *fp;
			fp = (struct trapframe *)curproc->p_md.md_regs;
			fp->tf_eflags |= PSL_IOPL;
		}
		break;
	default:
		break;
	}
	return (0);
}

/*ARGSUSED*/
int
mmclose(dev, flag, mode)
	dev_t dev;
	int flag, mode;
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
	register int o;
	register u_int c, v;
	register struct iovec *iov;
	int error = 0;
	static int physlock;

	if (minor(dev) == 0) {
		/* lock against other uses of shared vmempage */
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
			v = uio->uio_offset;
			pmap_enter(kernel_pmap, (vm_offset_t)vmmap,
			    trunc_page(v), uio->uio_rw == UIO_READ ? 
			    VM_PROT_READ : VM_PROT_WRITE, TRUE);
			o = (int)uio->uio_offset & PGOFSET;
			c = min(uio->uio_resid, (u_int)(NBPG - o));
			error = uiomove((caddr_t)vmmap + o, (int)c, uio);
			pmap_remove(kernel_pmap, (vm_offset_t)vmmap,
			    (vm_offset_t)vmmap + NBPG);
			continue;

/* minor device 1 is kernel memory */
		case 1:
			v = uio->uio_offset;
			c = min(iov->iov_len, MAXPHYS);
			if (!kernacc((caddr_t)v, c,
			    uio->uio_rw == UIO_READ ? B_READ : B_WRITE))
				return(EFAULT);
			error = uiomove(v, (int)c, uio);
			continue;

/* minor device 2 is EOF/RATHOLE */
		case 2:
			if (uio->uio_rw == UIO_WRITE)
				uio->uio_resid = 0;
			return (0);

/* minor device 12 (/dev/zero) is source of nulls on read, rathole on write */
		case 12:
			if (uio->uio_rw == UIO_WRITE) {
				c = iov->iov_len;
				break;
			}
			if (zeropage == NULL) {
				zeropage = (caddr_t)
				    malloc(CLBYTES, M_TEMP, M_WAITOK);
				bzero(zeropage, CLBYTES);
			}
			c = min(iov->iov_len, CLBYTES);
			error = uiomove(zeropage, (int)c, uio);
			continue;

		default:
			return (ENXIO);
		}
		if (error)
			break;
		iov->iov_base += c;
		iov->iov_len -= c;
		uio->uio_offset += c;
		uio->uio_resid -= c;
	}
	if (minor(dev) == 0) {
		if (physlock > 1)
			wakeup((caddr_t)&physlock);
		physlock = 0;
	}
	return (error);
}

int
mmmmap(dev, offset)
	dev_t dev;
	int offset;
{
	struct proc *p = curproc;	/* XXX */
	struct pcred *pc = p->p_cred;

	switch (minor(dev)) {
/* minor device 0 is physical memory */
	case 0:
		if (offset > ctob(physmem) &&
		    suser(pc->pc_ucred, &p->p_acflag) != 0)
			return -1;
		return i386_btop(offset);

/* minor device 1 is kernel memory */
	case 1:
		/* XXX - writability, executability checks? */
		if (!kernacc((caddr_t)offset, NBPG, B_READ))
			return -1;
		return i386_btop(vtophys(offset));
	default:
		return -1;
	}
}
