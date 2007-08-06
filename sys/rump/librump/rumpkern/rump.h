/*	$NetBSD: rump.h,v 1.2.2.2 2007/08/06 22:20:58 pooka Exp $	*/

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

#ifndef _SYS_RUMP_H_
#define _SYS_RUMP_H_

#include <sys/param.h>
#include <sys/mount.h>

#include <uvm/uvm.h>
#include <uvm/uvm_object.h>
#include <uvm/uvm_page.h>

struct lwp;
extern struct lwp *curlwp;
extern kauth_cred_t rump_cred;
extern struct vmspace rump_vmspace;

#define UIO_VMSPACE_SYS (&rump_vmspace)

struct rump_specpriv {
	char	rsp_path[MAXPATHLEN+1];
	int	rsp_fd;
};

#define RUMP_UBC_MAGIC_WINDOW (void *)0x37

void abort(void);

void	rump_init(void);
void	rump_mountinit(struct mount **, struct vfsops *);
void	rump_mountdestroy(struct mount *);

struct componentname	*rump_makecn(u_long, u_long,
				    const char *, size_t, struct lwp *);
void			rump_freecn(struct componentname *, int);

void	rump_putnode(struct vnode *);
int	rump_recyclenode(struct vnode *);

int	rump_ubc_magic_uiomove(size_t, struct uio *);
int 	rump_vopwrite_fault(struct vnode *, voff_t, size_t, kauth_cred_t);

int	rump_fakeblk_register(const char *);
int	rump_fakeblk_find(const char *);
void	rump_fakeblk_deregister(const char *);

void		rumpvm_init(void);
struct vm_page	*rumpvm_findpage(struct uvm_object *, voff_t);
struct vm_page	*rumpvm_makepage(struct uvm_object *, voff_t, int);
void		rumpvm_freepage(struct uvm_object *, struct vm_page *);

#endif /* _SYS_RUMP_H_ */
