/*	$NetBSD: ctrace.h,v 1.2.2.2 2018/04/16 01:59:48 pgoyette Exp $	*/

/* trace.h

   Definitions for dhcp tracing facility... */

/*
 * Copyright (c) 2004-2017 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 2001-2003 by Internet Software Consortium
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   950 Charter Street
 *   Redwood City, CA 94063
 *   <info@isc.org>
 *   https://www.isc.org/
 *
 */

typedef struct {
	struct in_addr primary_address;
	u_int32_t index;
	struct hardware hw_address;
	char name [IFNAMSIZ];
} trace_interface_packet_t;

typedef struct {
	u_int32_t index;
	struct iaddr from;
	u_int16_t from_port;
	struct hardware hfrom;
	u_int8_t havehfrom;
} trace_inpacket_t;

typedef struct {
	u_int32_t index;
	struct iaddr from;
	struct iaddr to;
	u_int16_t to_port;
	struct hardware hto;
	u_int8_t havehto;
} trace_outpacket_t;

void trace_interface_register (trace_type_t *, struct interface_info *);
void trace_interface_input (trace_type_t *, unsigned, char *);
void trace_interface_stop (trace_type_t *);
void trace_inpacket_stash (struct interface_info *,
			   struct dhcp_packet *, unsigned, unsigned int,
			   struct iaddr, struct hardware *);
void trace_inpacket_input (trace_type_t *, unsigned, char *);
void trace_inpacket_stop (trace_type_t *);
void trace_outpacket_input (trace_type_t *, unsigned, char *);
void trace_outpacket_stop (trace_type_t *);
ssize_t trace_packet_send (struct interface_info *,
			   struct packet *, struct dhcp_packet *, size_t, 
			   struct in_addr,
			   struct sockaddr_in *, struct hardware *);
void trace_icmp_input_input (trace_type_t *, unsigned, char *);
void trace_icmp_input_stop (trace_type_t *);
void trace_icmp_output_input (trace_type_t *, unsigned, char *);
void trace_icmp_output_stop (trace_type_t *);
void trace_seed_stash (trace_type_t *, unsigned);
void trace_seed_input (trace_type_t *, unsigned, char *);
void trace_seed_stop (trace_type_t *);
