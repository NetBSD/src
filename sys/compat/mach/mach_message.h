/*	$NetBSD: mach_message.h,v 1.1.4.3 2002/11/11 22:07:23 nathanw Exp $	 */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#ifndef	_MACH_MESSAGE_H_
#define	_MACH_MESSAGE_H_

typedef unsigned int mach_msg_bits_t;
typedef unsigned int mach_msg_size_t;
typedef unsigned int mach_msg_id_t;
typedef unsigned int mach_msg_timeout_t;
typedef unsigned int mach_msg_option_t;
typedef unsigned int mach_msg_type_name_t;
typedef unsigned int mach_msg_type_number_t;

/*
 * Options
 */
#define MACH_MSG_OPTION_NONE	0x00000000
#define	MACH_SEND_MSG		0x00000001
#define	MACH_RCV_MSG		0x00000002
#define MACH_RCV_LARGE		0x00000004
#define MACH_SEND_TIMEOUT	0x00000010
#define MACH_SEND_INTERRUPT	0x00000040
#define MACH_SEND_CANCEL	0x00000080
#define MACH_RCV_TIMEOUT	0x00000100
#define MACH_RCV_NOTIFY		0x00000200
#define MACH_RCV_INTERRUPT	0x00000400
#define MACH_RCV_OVERWRITE	0x00001000
#define MACH_SEND_ALWAYS	0x00010000
#define MACH_SEND_TRAILER	0x00020000	

#define MACH_MSG_OPTION_BITS	"\177\20" \
    "b\00send_msg\0b\01rcv_msg\0" \
    "b\02rcv_large\0b\03invalid[0x8]\0" \
    "b\04send_timeout\0b05invalid[0x20]\0" \
    "b\06send_interrupt\0b\05send_cancel\0" \
    "b\06rcv_timeout\0b\07rcv_notify\0" \
    "b\10rcv_interrupt\0b\11invalid[0x800]\0" \
    "b\12rcv_overwrite\0b\13invalid[0x2000]\0" \
    "b\14invalid[0x4000]\0b\15invalid[0x8000]\0" \
    "b\16send_always\0b\17send_trailer\0"

#define MACH_MSGH_BITS_REMOTE_MASK	0x000000ff
#define MACH_MSGH_BITS_LOCAL_MASK	0x0000ff00
#define MACH_MSGH_BITS_COMPLEX		0x80000000
#define MACH_MSGH_LOCAL_BITS(bits)	(((bits) >> 8) & 0xff)
#define MACH_MSGH_REMOTE_BITS(bits)	((bits) & 0xff)
#define MACH_MSGH_REPLY_LOCAL_BITS(bits)	(((bits) << 8) & 0xff00)

#define MACH_MSG_TYPE_MOVE_RECEIVE	16
#define MACH_MSG_TYPE_MOVE_SEND		17
#define MACH_MSG_TYPE_MOVE_SEND_ONCE	18
#define MACH_MSG_TYPE_COPY_SEND		19
#define MACH_MSG_TYPE_MAKE_SEND		20
#define MACH_MSG_TYPE_MAKE_SEND_ONCE	21
#define MACH_MSG_TYPE_COPY_RECEIVE	22

typedef unsigned int mach_msg_copy_options_t;

#define MACH_MSG_PHYSICAL_COPY		0
#define MACH_MSG_VIRTUAL_COPY   	1
#define MACH_MSG_ALLOCATE		2
#define MACH_MSG_OVERWRITE		3
#define MACH_MSG_KALLOC_COPY_T		4
#define MACH_MSG_PAGE_LIST_COPY_T	5

typedef unsigned int mach_msg_descriptor_type_t;

#define MACH_MSG_PORT_DESCRIPTOR 		0
#define MACH_MSG_OOL_DESCRIPTOR  		1
#define MACH_MSG_OOL_PORTS_DESCRIPTOR 		2
#define MACH_MSG_OOL_VOLATILE_DESCRIPTOR  	3

typedef	struct {
	mach_msg_bits_t	msgh_bits;
	mach_msg_size_t	msgh_size;
	mach_port_t	msgh_remote_port;
	mach_port_t	msgh_local_port;
	mach_msg_size_t msgh_reserved;
	mach_msg_id_t	msgh_id;
} mach_msg_header_t;

typedef u_int32_t mach_msg_trailer_type_t;
typedef u_int32_t mach_msg_trailer_size_t;
typedef struct { 
	mach_msg_trailer_type_t       msgh_trailer_type;
	mach_msg_trailer_size_t       msgh_trailer_size;
} mach_msg_trailer_t;

typedef struct {
	void*			pad1;
	mach_msg_size_t		pad2;
	unsigned int		pad3 : 24;
	mach_msg_descriptor_type_t type : 8;
} mach_msg_type_descriptor_t;

typedef struct {
	mach_port_t		name;
	mach_msg_size_t		pad1;
	unsigned int		pad2 : 16;
	mach_msg_type_name_t	disposition : 8;
	mach_msg_descriptor_type_t type : 8;
} mach_msg_port_descriptor_t;

typedef struct {
	void *			address;
	mach_msg_size_t		count;
	mach_boolean_t		deallocate: 8;
	mach_msg_copy_options_t	copy: 8;
	mach_msg_type_name_t	disposition : 8;
	mach_msg_descriptor_type_t type : 8;
} mach_msg_ool_ports_descriptor_t;

typedef struct {
	mach_msg_size_t	msgh_descriptor_count;
} mach_msg_body_t;

struct mach_subsystem_namemap {
	int	map_id;
	int	(*map_handler) __P((struct proc *, mach_msg_header_t *));
	const char	*map_name;
};
extern struct mach_subsystem_namemap mach_namemap[];

#endif /* !_MACH_MESSAGE_H_ */
