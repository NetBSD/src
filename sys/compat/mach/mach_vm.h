/*	$NetBSD: mach_vm.h,v 1.15 2003/09/06 23:52:27 manu Exp $ */

/*-
 * Copyright (c) 2002-2003 The NetBSD Foundation, Inc.
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

#define	MACH_ALTERNATE_LOAD_SITE	1
#define MACH_NEW_LOCAL_SHARED_REGIONS	2
#define MACH_QUERY_IS_SYSTEM_REGION	4
#define MACH_SF_PREV_LOADED		1
#define MACH_SYSTEM_REGION_BACKED	2

#define MACH_VM_PROT_COW	0x8
#define MACH_VM_PROT_ZF		0x10

typedef struct mach_sf_mapping {
	mach_vm_offset_t mapping_offset;
	mach_vm_size_t size;
	mach_vm_offset_t file_offset;
	mach_vm_prot_t protection;
	mach_vm_offset_t cksum;
} mach_sf_mapping_t;

struct mach_vm_region_basic_info {
	mach_vm_prot_t protection;
	mach_vm_prot_t max_protection;
	mach_vm_inherit_t inheritance;
	mach_boolean_t shared;
	mach_boolean_t reserved;
	mach_vm_offset_t offset;
	mach_vm_behavior_t behavior;
	unsigned short user_wired_count;
};

/* There is no difference between 32 and 64 bits versions */
struct mach_vm_region_basic_info_64 {
	mach_vm_prot_t protection;
	mach_vm_prot_t max_protection;
	mach_vm_inherit_t inheritance;
	mach_boolean_t shared;
	mach_boolean_t reserved;
	mach_vm_offset_t offset;
	mach_vm_behavior_t behavior;
	unsigned short user_wired_count;
};

/* mach_vm_behavior_t values */
#define MACH_VM_BEHAVIOR_DEFAULT 0
#define MACH_VM_BEHAVIOR_RANDOM 1
#define MACH_VM_BEHAVIOR_SEQUENTIAL 2
#define MACH_VM_BEHAVIOR_RSEQNTL 3
#define MACH_VM_BEHAVIOR_WILLNEED 4
#define MACH_VM_BEHAVIOR_DONTNEED 5

/* vm_map */
#define MACH_VM_INHERIT_SHARE 0
#define MACH_VM_INHERIT_COPY 1
#define MACH_VM_INHERIT_NONE 2
#define MACH_VM_INHERIT_DONATE_COPY 3
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
#define MACH_VM_FLAGS_ANYWHERE 1
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

/* vm_wire */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_msg_body_t req_body;
	mach_msg_port_descriptor_t req_task;
	mach_ndr_record_t req_ndr;
	mach_vm_address_t req_address;
	mach_vm_size_t req_size;
	mach_vm_prot_t req_access;
} mach_vm_wire_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_vm_wire_reply_t;

/* vm_protect */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_vm_address_t req_addr;
	mach_vm_size_t req_size;
	mach_boolean_t req_set_maximum;
	mach_vm_prot_t req_prot;
} mach_vm_protect_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_vm_protect_reply_t;

/* vm_inherit */
typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_vm_address_t req_addr;
	mach_vm_size_t req_size;
	mach_vm_inherit_t req_inh;
} mach_vm_inherit_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_vm_inherit_reply_t;

/* vm_make_memory_entry */
typedef struct {
	mach_msg_header_t req_msgh;
	mach_msg_body_t req_body;
	mach_msg_port_descriptor_t req_parent_entry;
	mach_ndr_record_t req_ndr;
	mach_vm_size_t req_size;
	mach_vm_offset_t req_offset;
	mach_vm_prot_t req_perm;
} mach_vm_make_memory_entry_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_port_descriptor_t rep_obj_handle;
	mach_ndr_record_t rep_ndr;
	mach_vm_size_t rep_size;
	mach_msg_trailer_t rep_trailer;
} mach_vm_make_memory_entry_reply_t;

/* vm_region */

#define MACH_VM_REGION_BASIC_INFO 10
typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_vm_address_t req_addr;
	mach_vm_region_flavor_t req_flavor;
	mach_msg_type_number_t req_count;
} mach_vm_region_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_ool_descriptor_t rep_obj;
	mach_ndr_record_t rep_ndr;
	mach_msg_type_number_t rep_addr;
	mach_vm_size_t rep_size;
	mach_msg_type_number_t rep_count;
	int rep_info[9];
	mach_msg_trailer_t rep_trailer;
} mach_vm_region_reply_t;

/* vm_region_64 */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_vm_address_t req_addr;
	mach_vm_region_flavor_t req_flavor;
	mach_msg_type_number_t req_count;
} mach_vm_region_64_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_ool_descriptor_t rep_obj;
	mach_ndr_record_t rep_ndr;
	mach_vm_address_t rep_addr;
	mach_vm_size_t rep_size;
	mach_msg_type_number_t rep_count;
	int rep_info[10];
	mach_msg_trailer_t rep_trailer;
} mach_vm_region_64_reply_t;

/* vm_msync */
#define MACH_VM_SYNC_ASYNCHRONOUS 0x01
#define MACH_VM_SYNC_SYNCHRONOUS 0x02
#define MACH_VM_SYNC_INVALIDATE 0x04
#define MACH_VM_SYNC_KILLPAGES 0x08
#define MACH_VM_SYNC_DEACTIVATE 0x10
typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_vm_address_t req_addr;
	mach_vm_size_t req_size;
	mach_vm_sync_t req_flags;
} mach_vm_msync_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_vm_msync_reply_t;

/* vm_copy */
typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_vm_address_t req_src;
	mach_vm_size_t req_size;
	mach_vm_address_t req_addr;
} mach_vm_copy_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_vm_copy_reply_t;


int mach_vm_map(struct mach_trap_args *);
int mach_vm_allocate(struct mach_trap_args *);
int mach_vm_deallocate(struct mach_trap_args *);
int mach_vm_wire(struct mach_trap_args *);
int mach_vm_protect(struct mach_trap_args *);
int mach_vm_inherit(struct mach_trap_args *);
int mach_vm_make_memory_entry(struct mach_trap_args *);
int mach_vm_region(struct mach_trap_args *);
int mach_vm_region_64(struct mach_trap_args *);
int mach_vm_msync(struct mach_trap_args *);
int mach_vm_copy(struct mach_trap_args *);

#endif /* _MACH_VM_H_ */
