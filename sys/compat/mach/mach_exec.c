/*	$NetBSD: mach_exec.c,v 1.28 2003/03/29 11:04:08 manu Exp $	 */

/*-
 * Copyright (c) 2001-2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas and Emmanuel Dreyfus.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mach_exec.c,v 1.28 2003/03/29 11:04:08 manu Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/queue.h>
#include <sys/exec_macho.h>
#include <sys/malloc.h>

#include <sys/syscall.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_param.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_semaphore.h>
#include <compat/mach/mach_notify.h>
#include <compat/mach/mach_exec.h>

static int mach_cold = 1; /* Have we initialized COMPAT_MACH structures? */

static void mach_e_proc_exec(struct proc *, struct exec_package *);
static void mach_e_proc_fork(struct proc *, struct proc *);
static void mach_init(void);

extern char sigcode[], esigcode[];
extern struct sysent sysent[];
#ifdef SYSCALL_DEBUG
extern const char * const syscallnames[];
#endif
#ifndef __HAVE_SYSCALL_INTERN
void syscall(void);
#else
void mach_syscall_intern(struct proc *);
#endif

const struct emul emul_mach = {
	"mach",
	"/emul/mach",
#ifndef __HAVE_MINIMAL_EMUL
	0,
	0,
	SYS_syscall,
	SYS_NSYSENT,
#endif
	sysent,
#ifdef SYSCALL_DEBUG
	syscallnames,
#else
	NULL,
#endif
	sendsig,
	trapsignal,
	sigcode,
	esigcode,
	setregs,
	mach_e_proc_exec,
	mach_e_proc_fork,
	mach_e_proc_exit,
#ifdef __HAVE_SYSCALL_INTERN
	mach_syscall_intern,
#else
	syscall,
#endif
	NULL,
	NULL,
};

/*
 * Copy arguments onto the stack in the normal way, but add some
 * extra information in case of dynamic binding.
 * XXX This needs a cleanup: it is not used anymore by the Darwin 
 * emulation, and it probably contains Darwin specific bits. 
 */
int
exec_mach_copyargs(p, pack, arginfo, stackp, argp)
	struct proc *p;
	struct exec_package *pack;
	struct ps_strings *arginfo;
	char **stackp;
	void *argp;
{
	struct exec_macho_emul_arg *emea;
	struct exec_macho_object_header *macho_hdr;
	size_t len;
	size_t zero = 0;
	int error;
	
	*stackp = (char *)(((unsigned long)*stackp - 1) & ~0xfUL);

	emea = (struct exec_macho_emul_arg *)pack->ep_emul_arg;
	macho_hdr = (struct exec_macho_object_header *)emea->macho_hdr;
	if ((error = copyout(&macho_hdr, *stackp, sizeof(macho_hdr))) != 0) 
		return error;
	
	*stackp += sizeof(macho_hdr);

	if ((error = copyargs(p, pack, arginfo, stackp, argp)) != 0) {
		DPRINTF(("mach: copyargs failed\n"));
		return error;
	}

	if ((error = copyout(&zero, *stackp, sizeof(zero))) != 0)
		return error;
	*stackp += sizeof(zero);

	if ((error = copyoutstr(emea->filename, 
	    *stackp, MAXPATHLEN, &len)) != 0) {
		DPRINTF(("mach: copyout path failed\n"));
		return error;
	}
	*stackp += len + 1;

	/* We don't need this anymore */
	free(pack->ep_emul_arg, M_EXEC);
	pack->ep_emul_arg = NULL;

	len = len % sizeof(zero);
	if (len) {
		if ((error = copyout(&zero, *stackp, len)) != 0) 
			return error;
		*stackp += len;
	}

	if ((error = copyout(&zero, *stackp, sizeof(zero))) != 0) 
		return error;
	*stackp += sizeof(zero);

	return 0;
}

int
exec_mach_probe(path)
	char **path;
{
	*path = (char *)emul_mach.e_path;
	return 0;
}

static void 
mach_e_proc_exec(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	mach_e_proc_init(p, p->p_vmspace);

	return;
}

static void 
mach_e_proc_fork(p, parent)
	struct proc *p;
	struct proc *parent;
{
	struct mach_emuldata *med1;
	struct mach_emuldata *med2;

	p->p_emuldata = NULL;

	/* Use parent's vmspace because our vmspace may not be setup yet */
	mach_e_proc_init(p, parent->p_vmspace);

	med1 = p->p_emuldata;
	med2 = parent->p_emuldata;
	/* Nothing is inherited across forks in struct  mach_emuldata */

	return;
}

void 
mach_e_proc_init(p, vmspace)
	struct proc *p;
	struct vmspace *vmspace;
{
	struct mach_emuldata *med;

	/* 
	 * Initialize various things if needed. 
	 * XXX Not the best place for that. 
	 */
	if (mach_cold == 1)
		mach_init();

	if (!p->p_emuldata)
		p->p_emuldata = malloc(sizeof(struct mach_emuldata),
		    M_EMULDATA, M_WAITOK | M_ZERO);

	med = (struct mach_emuldata *)p->p_emuldata;

	LIST_INIT(&med->med_right);
	lockinit(&med->med_rightlock, PZERO|PCATCH, "mach_right", 0, 0);
	/* 
	 * For debugging purpose, it's convenient to have each process 
	 * using distinct port names, so we prefix the first port name
	 * by the PID. Darwin does not do that, but we can remove it 
	 * when we want, it will not hurt.
	 */
	med->med_nextright = p->p_pid << 16;

	med->med_kernel = mach_port_get();
	med->med_host = mach_port_get();

	med->med_kernel->mp_flags |= MACH_MP_INKERNEL;
	med->med_host->mp_flags |= MACH_MP_INKERNEL;

	med->med_kernel->mp_data = (void *)p;
	med->med_host->mp_data = (void *)p;

	med->med_kernel->mp_datatype = MACH_MP_PROC;
	med->med_host->mp_datatype = MACH_MP_PROC;

	/* Make sure they will not be deallocated */
	med->med_kernel->mp_refcount++;
	med->med_host->mp_refcount++;

	med->med_bootstrap = mach_bootstrap_port;
	med->med_bootstrap->mp_refcount++;

	bzero(med->med_exc, sizeof(med->med_exc));

	med->med_dirty_thid = 1;
	return;
}

void 
mach_e_proc_exit(p)
	struct proc *p;
{
	struct mach_emuldata *med;
	struct mach_right *mr;

	mach_semaphore_cleanup(p);

	med = (struct mach_emuldata *)p->p_emuldata;

	lockmgr(&med->med_rightlock, LK_EXCLUSIVE, NULL);
	while ((mr = LIST_FIRST(&med->med_right)) != NULL)
		mach_right_put_exclocked(mr, MACH_PORT_TYPE_ALL_RIGHTS);
	
	lockmgr(&med->med_rightlock, LK_RELEASE, NULL);

	if (--med->med_bootstrap->mp_refcount == 0)
		mach_port_put(med->med_bootstrap);
	if (--med->med_kernel->mp_refcount == 0) 
		mach_port_put(med->med_kernel);
	if (--med->med_host->mp_refcount == 0)  
		mach_port_put(med->med_host);  

	free(med, M_EMULDATA);
	p->p_emuldata = NULL;

	return;
}

static void
mach_init(void)
{
	mach_semaphore_init();
	mach_message_init();
	mach_port_init();

	mach_cold = 0;

	return;
}
