/*	$NetBSD: darwin_iohidsystem.c,v 1.6 2003/05/14 18:28:05 manu Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
__KERNEL_RCSID(0, "$NetBSD: darwin_iohidsystem.c,v 1.6 2003/05/14 18:28:05 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/signal.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/kthread.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_map.h>
#include <uvm/uvm.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_errno.h>
#include <compat/mach/mach_iokit.h>

#include <compat/darwin/darwin_iokit.h>
#include <compat/darwin/darwin_iohidsystem.h>

static struct uvm_object *darwin_iohidsystem_shmem = NULL;
static void darwin_iohidsystem_shmeminit(vaddr_t);
static void darwin_iohidsystem_thread(void *);

struct mach_iokit_devclass darwin_iohidsystem_devclass = {
	"<dict ID=\"0\"><key>IOProviderClass</key>"
	    "<string ID=\"1\">IOHIDSystem</string></dict>",
	NULL,
	NULL,
	darwin_iohidsystem_connect_method_scalari_scalaro,
	NULL,
	NULL,
	darwin_iohidsystem_connect_map_memory,
	"IOHIDSystem",
};

int
darwin_iohidsystem_connect_method_scalari_scalaro(args)
	struct mach_trap_args *args;
{
	mach_io_connect_method_scalari_scalaro_request_t *req = args->smsg;
	mach_io_connect_method_scalari_scalaro_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;

#ifdef DEBUG_DARWIN
	printf("darwin_iohidsystem_connect_method_scalari_scalaro()\n");
#endif
	rep->rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_outcount = 0;
	rep->rep_out[rep->rep_outcount + 1] = 8; /* XXX Trailer */

	*msglen = sizeof(*rep) - ((4096 + rep->rep_outcount) * sizeof(int));
	return 0;
}

int
darwin_iohidsystem_connect_map_memory(args)
	struct mach_trap_args *args;
{
	mach_io_connect_map_memory_request_t *req = args->smsg;
	mach_io_connect_map_memory_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct proc *p = args->l->l_proc;
	struct proc *newpp;
	int error;
	size_t memsize;
	vaddr_t pvaddr;
	vaddr_t kvaddr;

#ifdef DEBUG_DARWIN
	printf("darwin_iohidsystem_connect_map_memory()\n");
#endif
	memsize = round_page(sizeof(struct darwin_iohidsystem_shmem));

	/* If it has not been used yet, initialize it */
	if (darwin_iohidsystem_shmem == NULL) {
		darwin_iohidsystem_shmem = uao_create(memsize, 0);

		error = uvm_map(kernel_map, &kvaddr, memsize, 
		    darwin_iohidsystem_shmem, 0, PAGE_SIZE,
		    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW, 
		    UVM_INH_SHARE, UVM_ADV_RANDOM, 0));
		if (error != 0) {
			uao_detach(darwin_iohidsystem_shmem);
			darwin_iohidsystem_shmem = NULL;
			return error;
		}

		error = uvm_map_pageable(kernel_map, kvaddr, 
		    kvaddr + memsize, FALSE, 0);
		if (error != 0) {
			uao_detach(darwin_iohidsystem_shmem);
			darwin_iohidsystem_shmem = NULL;
			return error;
		}

		darwin_iohidsystem_shmeminit(kvaddr);

		kthread_create1(darwin_iohidsystem_thread, 
		    (void *)kvaddr, &newpp, "iohidsystem");
	}

	uao_reference(darwin_iohidsystem_shmem);
	pvaddr = VM_DEFAULT_ADDRESS(p->p_vmspace->vm_daddr, memsize);

	if ((error = uvm_map(&p->p_vmspace->vm_map, &pvaddr, 
	    memsize, darwin_iohidsystem_shmem, 0, PAGE_SIZE, 
	    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW,
	    UVM_INH_SHARE, UVM_ADV_RANDOM, 0))) != 0)
		return mach_msg_error(args, error);

#ifdef DEBUG_DARWIN
	printf("pvaddr = 0x%08lx\n", (long)pvaddr);
#endif
	rep->rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_retval = 0;
	rep->rep_addr = pvaddr;
	rep->rep_len = sizeof(struct darwin_iohidsystem_shmem);
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);

	return 0;
}

static void
darwin_iohidsystem_thread(shmem)
	void *shmem;
{
#ifdef DEBUG_DARWIN
	printf("darwin_iohidsystem_thread: start\n");
#endif
	/* 
	 * This will receive wscons events and modify the IOHIDSystem
	 * shared page. But for now it just sleep forever.
	 */
	(void)tsleep(shmem, PZERO | PCATCH, "iohidsystem", 0);
#ifdef DEBUG_DARWIN
	printf("darwin_iohidsystem_thread: exit\n");
#endif
	return;
};

static void
darwin_iohidsystem_shmeminit(kvaddr)
	vaddr_t kvaddr;
{
	struct darwin_iohidsystem_shmem *shmem;
	struct darwin_iohidsystem_evglobals *evglobals;

	shmem = (struct darwin_iohidsystem_shmem *)kvaddr;
	shmem->dis_global_offset = 
	    (size_t)&shmem->dis_evglobals - (size_t)&shmem->dis_global_offset;
	shmem->dis_private_offset = 
	    shmem->dis_global_offset + sizeof(*evglobals);

	evglobals = &shmem->dis_evglobals;
	evglobals->die_struct_size = sizeof(*evglobals);

	return;
}
