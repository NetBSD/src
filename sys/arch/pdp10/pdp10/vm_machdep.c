/*	$NetBSD: vm_machdep.c,v 1.1 2003/08/19 10:55:00 ragge Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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

#include <sys/param.h>
#include <sys/signalvar.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

void
pagemove(caddr_t from, caddr_t to, size_t size)
{
	panic("pagemove");
}

int
cpu_coredump(struct lwp *p, struct vnode *vp,
    struct ucred *cred, struct core *chdr)
{
	return 0;
}

int
sys_sysarch(struct lwp *p, void *v, register_t *retval)
{
	return (ENOSYS);
}

void
vmapbuf(struct buf *bp, vsize_t len)
{
	panic("vmapbuf");
}

void
vunmapbuf(struct buf *bp, vsize_t len)
{
	panic("vunmapbuf");
}

void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	panic("cpu_lwp_fork");
}

void  
cpu_setfunc(struct lwp *l, void (*func)(void *), void *arg)
{
	panic("cpu_setfunc");
}

/*    
 * Start a new LWP
 */
void  
startlwp(void *arg)
{
	panic("startlwp");
}
