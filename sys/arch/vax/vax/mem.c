/*	$NetBSD: mem.c,v 1.4 1995/03/30 21:25:25 ragge Exp $	*/

/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

 /* All bugs are subject to removal without further notice */
		


#include <sys/param.h>
#include <sys/buf.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include "lib/libkern/libkern.h"
#include "vm/vm.h"
#include "vm/vm_kern.h"
#include "machine/pte.h"
#include "machine/mtpr.h"

#define TRUE 1

extern int *pte_cmap;
extern unsigned int v_cmap,avail_end;

mmrw(dev_t dev, struct uio *uio, int flags) {

  unsigned long  b;
  unsigned long  c,o;
	caddr_t zbuf = NULL;

  int error = 0;
  register struct iovec *iov;

  while (uio->uio_resid > 0 && error == 0) {
    iov = uio->uio_iov;
    if (iov->iov_len == 0) {
      uio->uio_iov++;
      uio->uio_iovcnt--;
      if (uio->uio_iovcnt < 0) {
	panic("mmrw");
      }
      continue;
    }
    switch (minor(dev)) {
    case 0:   /* Minor dev 0 is physical memory */
      if((b=uio->uio_offset) > avail_end) {
	return(EFAULT);
      }

      pmap_enter(pmap_kernel(), v_cmap, trunc_page(b),
		 uio->uio_rw == UIO_READ ?
		 VM_PROT_READ : VM_PROT_WRITE, TRUE);
      o = (int)uio->uio_offset & PGOFSET;
      c = (u_int)(NBPG - ((int)iov->iov_base & PGOFSET));
      c = min(c, (u_int)(NBPG - o));
      c = min(c, (u_int)iov->iov_len);
      error = uiomove((caddr_t)(v_cmap+o), (int)c, uio);
      pmap_remove(pmap_kernel(), v_cmap, v_cmap+NBPG);
      continue;

/* minor device 1 is kernel memory */
                case 1:
                        c = iov->iov_len;
                        if (!kernacc((caddr_t)(long)uio->uio_offset, c,
                            uio->uio_rw == UIO_READ ? B_READ : B_WRITE))
                                return(EFAULT);
                        error = uiomove((caddr_t)(long)uio->uio_offset,
                            (int)c, uio);
                        continue;


    case 2:   /* Minor dev 2 is EOF/RATHOLE ,ie /dev/null */
      if (uio->uio_rw == UIO_READ) {
	return (0);
      }
      c = iov->iov_len;
      break;

/* minor device 12 (/dev/zero) is source of nulls on read, rathole on write */
                case 12:
                        if (uio->uio_rw == UIO_WRITE) {
                                c = iov->iov_len;
                                break;
                        }
                        if (zbuf == NULL) {
                                zbuf = (caddr_t)
                                    malloc(CLBYTES, M_TEMP, M_WAITOK);
                                bzero(zbuf, CLBYTES);
                        }
                        c = min(iov->iov_len, CLBYTES);
                        error = uiomove(zbuf, (int)c, uio);
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
	if (zbuf)
		free(zbuf, M_TEMP);
  return (error);
}
