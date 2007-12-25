/*	$NetBSD: rump_private.h,v 1.7 2007/12/25 18:33:48 perry Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYS_RUMP_PRIVATE_H_
#define _SYS_RUMP_PRIVATE_H_

#include <sys/param.h>
#include <sys/types.h>

#include <sys/disklabel.h>
#include <sys/mount.h>
#include <sys/vnode.h>

#include <uvm/uvm.h>
#include <uvm/uvm_object.h>
#include <uvm/uvm_page.h>

#include "rump.h"

#if 0
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

struct lwp;
extern kauth_cred_t rump_cred;
extern struct vmspace rump_vmspace;

extern kmutex_t rump_giantlock;

#define UIO_VMSPACE_SYS (&rump_vmspace)

struct rump_specpriv {
	char	rsp_path[MAXPATHLEN+1];
	int	rsp_fd;

	struct partition *rsp_curpi;
	struct partition rsp_pi;
	struct disklabel rsp_dl;
};

#define RUMP_UBC_MAGIC_WINDOW (void *)0x37

void abort(void) __dead;

void	rump_putnode(struct vnode *);
int	rump_recyclenode(struct vnode *);

struct ubc_window;
int	rump_ubc_magic_uiomove(void *, size_t, struct uio *, int *,
			       struct ubc_window *);

void		rumpvm_init(void);
void		rump_sleepers_init(void);
struct vm_page	*rumpvm_makepage(struct uvm_object *, voff_t);

void		rumpvm_enterva(vaddr_t addr, struct vm_page *);
void		rumpvm_flushva(void);

#endif /* _SYS_RUMP_PRIVATE_H_ */
