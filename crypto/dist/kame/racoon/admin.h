/*	$KAME: admin.h,v 1.8 2000/10/04 17:40:58 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* command for administration. */
/* NOTE: host byte order. */
struct admin_com {
	u_int16_t ac_len;	/* total packet length including data */
	u_int16_t ac_cmd;
	int16_t ac_errno;
	u_int16_t ac_proto;
};

/*
 * No data follows as the data.
 * These don't use proto field.
 */
#define ADMIN_RELOAD_CONF	0x0001
#define ADMIN_SHOW_SCHED	0x0002

/*
 * No data follows as the data.
 * These use proto field.
 */
#define ADMIN_SHOW_SA		0x0101
#define ADMIN_FLUSH_SA		0x0102

/*
 * The admin_com_indexes follows, see below.
 */
#define ADMIN_DELETE_SA		0x0201
#define ADMIN_ESTABLISH_SA	0x0202

/* the value of proto */
#define ADMIN_PROTO_ISAKMP	0x01ff
#define ADMIN_PROTO_IPSEC	0x02ff
#define ADMIN_PROTO_AH		0x0201
#define ADMIN_PROTO_ESP		0x0202
#define ADMIN_PROTO_INTERNAL	0x0301

struct admin_com_indexes {
	u_int8_t prefs;
	u_int8_t prefd;
	u_int8_t ul_proto;
	u_int8_t reserved;
	struct sockaddr_storage src;
	struct sockaddr_storage dst;
};

extern int admin2pfkey_proto __P((u_int));
