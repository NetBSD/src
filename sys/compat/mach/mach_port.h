/*	$NetBSD: mach_port.h,v 1.18 2003/02/02 19:07:18 manu Exp $ */

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

#ifndef	_MACH_PORT_H_
#define	_MACH_PORT_H_

#define MACH_PORT_NULL			(struct mach_right *)0
#define MACH_PORT_DEAD			(struct mach_right *)-1

#define MACH_PORT_RIGHT_SEND		0
#define MACH_PORT_RIGHT_RECEIVE		1
#define MACH_PORT_RIGHT_SEND_ONCE	2
#define MACH_PORT_RIGHT_PORT_SET	3
#define MACH_PORT_RIGHT_DEAD_NAME	4
#define MACH_PORT_RIGHT_NUMBER		5

#define MACH_PORT_TYPE_SEND		(1 << (MACH_PORT_RIGHT_SEND + 16))
#define MACH_PORT_TYPE_RECEIVE		(1 << (MACH_PORT_RIGHT_RECEIVE + 16))
#define MACH_PORT_TYPE_SEND_ONCE	(1 << (MACH_PORT_RIGHT_SEND_ONCE + 16))
#define MACH_PORT_TYPE_PORT_SET		(1 << (MACH_PORT_RIGHT_PORT_SET + 16))
#define MACH_PORT_TYPE_DEAD_NAME	(1 << (MACH_PORT_RIGHT_DEAD_NAME + 16))
#define MACH_PORT_TYPE_PORT_RIGHTS \
    (MACH_PORT_TYPE_SEND | MACH_PORT_TYPE_RECEIVE | MACH_PORT_TYPE_SEND_ONCE)
#define MACH_PORT_TYPE_PORT_OR_DEAD \
    (MACH_PORT_TYPE_PORT_RIGHTS | MACH_PORT_TYPE_DEAD_NAME)
#define MACH_PORT_TYPE_ALL_RIGHTS \
    (MACH_PORT_TYPE_PORT_OR_DEAD|MACH_PORT_TYPE_PORT_SET)
#define MACH_PORT_TYPE_REF_RIGHTS \
    (MACH_PORT_TYPE_SEND | MACH_PORT_TYPE_SEND_ONCE | MACH_PORT_TYPE_DEAD_NAME)
	

/* port_deallocate */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_port_name_t req_name;
} mach_port_deallocate_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_port_deallocate_reply_t;

/* port_allocate */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_port_right_t req_right;
} mach_port_allocate_request_t;  

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_port_name_t rep_name;
	mach_msg_trailer_t rep_trailer;
} mach_port_allocate_reply_t;  

/* port_insert_right */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_msg_body_t req_body;
	mach_msg_port_descriptor_t req_poly;
	mach_ndr_record_t req_ndr;
	mach_port_name_t req_name;
} mach_port_insert_right_request_t;  

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_port_insert_right_reply_t;  

/* port_type */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_port_name_t req_name;
} mach_port_type_request_t;  

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_port_type_t rep_ptype;
	mach_msg_trailer_t rep_trailer;
} mach_port_type_reply_t;  

/* port_get_attributes */

typedef int mach_port_flavor_t;
typedef mach_natural_t mach_port_seqno_t; 
typedef mach_natural_t mach_port_mscount_t;
typedef mach_natural_t mach_port_msgcount_t;
typedef mach_natural_t mach_port_rights_t;
#define MACH_PORT_LIMITS_INFO 1
typedef struct mach_port_status {
	mach_port_name_t	mps_pset;
	mach_port_seqno_t	mps_seqno;
	mach_port_mscount_t	mps_mscount;
	mach_port_msgcount_t	mps_qlimit;
	mach_port_msgcount_t	mps_msgcount;
	mach_port_rights_t	mps_sorights;
	mach_boolean_t		mps_srights;
	mach_boolean_t		mps_pdrequest;
	mach_boolean_t		mps_nsrequest;
	unsigned int		mps_flags;
} mach_port_status_t;
#define MACH_PORT_RECEIVE_STATUS 2
typedef struct mach_port_limits {
	mach_port_msgcount_t	mpl_qlimit;
} mach_port_limits_t;
#define MACH_PORT_DNREQUESTS_SIZE 3

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_port_name_t req_name;
	mach_port_flavor_t req_flavor;
	mach_msg_type_number_t req_count;
	mach_integer_t req_port_info[10];
} mach_port_set_attributes_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_port_set_attributes_reply_t;
	
/* port_insert_member */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_port_name_t req_name;
	mach_port_name_t req_pset;
} mach_port_insert_member_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_port_insert_member_reply_t;

/* port_move_member */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_port_name_t req_member;
	mach_port_name_t req_after;
} mach_port_move_member_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_port_move_member_reply_t;

/* port_destroy */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_port_name_t req_name;
} mach_port_destroy_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_port_destroy_reply_t;

int mach_port_deallocate(struct mach_trap_args *);
int mach_port_allocate(struct mach_trap_args *);
int mach_port_insert_right(struct mach_trap_args *);
int mach_port_type(struct mach_trap_args *);
int mach_port_set_attributes(struct mach_trap_args *);
int mach_port_insert_member(struct mach_trap_args *);
int mach_port_move_member(struct mach_trap_args *);
int mach_port_destroy(struct mach_trap_args *);

extern struct mach_port *mach_clock_port;
extern struct mach_port *mach_io_master_port;
extern struct mach_port *mach_bootstrap_port;
extern struct mach_port *mach_saved_bootstrap_port;

/* In-kernel Mach port right description */
struct mach_right {
	mach_port_t mr_name;		/* The right name */
	struct lwp *mr_lwp;		/* points back to struct lwp */
	int mr_type;			/* right type (recv, send, sendonce) */
	LIST_ENTRY(mach_right) mr_list; /* Right list for a process */
	int mr_refcount;		/* Reference count */

	/* Revelant only if the right is on a port set */
	LIST_HEAD(mr_set, mach_right) mr_set;
					/* The right set list */

	/* Revelant only if the right is not on a port set */
	struct mach_port *mr_port;	/* Port we have the right on */
	LIST_ENTRY(mach_right) mr_setlist; /* Set list */

	/* Revelant only if the right is part of a port set */
	struct mach_right *mr_sethead;	/* Points back to right set */
};

mach_port_t mach_right_newname(struct lwp *, mach_port_t);
struct mach_right *mach_right_get(struct mach_port *, 
    struct lwp *, int, mach_port_t);
void mach_right_put(struct mach_right *, int);
void mach_right_put_shlocked(struct mach_right *, int);
void mach_right_put_exclocked(struct mach_right *, int);
struct mach_right *mach_right_check(mach_port_t, struct lwp *, int);

/* In-kernel Mach port description */
struct mach_port {
	struct mach_right *mp_recv;	/* The receive right on this port */
	int mp_count;			/* Count of queued messages */
	TAILQ_HEAD(mp_msglist,		/* Queue pending messages */
	    mach_message) mp_msglist;
	struct lock mp_msglock;		/* Lock for the queue */
	int mp_refcount;		/* Reference count */
};

void mach_port_init(void);
struct mach_port *mach_port_get(void);
void mach_port_put(struct mach_port *);
void mach_remove_recvport(struct mach_port *);
void mach_add_recvport(struct mach_port *, struct lwp *);
int mach_port_check(struct mach_port *);
#ifdef DEBUG_MACH
void mach_debug_port(void);
#endif

#endif /* _MACH_PORT_H_ */
