/*	$NetBSD: mach_vm.h,v 1.2 2002/11/12 06:14:39 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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

#ifndef	_MACH_VM_H_
#define	_MACH_VM_H_


#include <sys/types.h> 
#include <sys/param.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>

/* vm_map */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_msg_body_t req_body;
	mach_msg_port_descriptor_t req_object;
	mach_ndr_record_t req_ndr;
	mach_vm_address_t req_address;
	mach_vm_size_t req_size;
	mach_vm_address_t req_mask;
	int req_flags;
	mach_vm_offset_t req_offset;
	mach_boolean_t req_copy;
	mach_vm_prot_t req_cur_protection;
	mach_vm_prot_t req_max_protection;
	mach_vm_inherit_t req_inherance;
} mach_vm_map_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_vm_address_t rep_address;
	mach_msg_trailer_t rep_trailer;
} mach_vm_map_reply_t;

/* vm_allocate */
typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_vm_address_t req_address;
	mach_vm_size_t req_size;
	int req_flags;
} mach_vm_allocate_request_t;


typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_vm_address_t rep_address;
	mach_msg_trailer_t rep_trailer;
} mach_vm_allocate_reply_t;

/* vm_deallocate */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_vm_address_t req_address;
	mach_vm_size_t req_size;
} mach_vm_deallocate_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_vm_deallocate_reply_t;

int mach_vm_map __P((struct proc *, mach_msg_header_t *));
int mach_vm_allocate __P((struct proc *, mach_msg_header_t *));
int mach_vm_deallocate __P((struct proc *, mach_msg_header_t *));

#endif /* _MACH_VM_H_ */
