/* salloc.c

   Memory allocation for the DHCP server... */

/*
 * Copyright (c) 1996-2000 Internet Software Consortium.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for the Internet Software Consortium
 * by Ted Lemon in cooperation with Vixie Enterprises and Nominum, Inc.
 * To learn more about the Internet Software Consortium, see
 * ``http://www.isc.org/''.  To learn more about Vixie Enterprises,
 * see ``http://www.vix.com''.   To learn more about Nominum, Inc., see
 * ``http://www.nominum.com''.
 */

#ifndef lint
static char copyright[] =
"$Id: salloc.c,v 1.1.1.1.4.2 2000/06/22 18:00:56 minoura Exp $ Copyright (c) 1996-2000 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"
#include <omapip/omapip_p.h>

struct lease *new_leases (n, file, line)
	unsigned n;
	const char *file;
	int line;
{
	struct lease *rval = dmalloc (n * sizeof (struct lease), file, line);
	return rval;
}

OMAPI_OBJECT_ALLOC (lease, struct lease, dhcp_type_lease)
OMAPI_OBJECT_ALLOC (class, struct class, dhcp_type_class)
OMAPI_OBJECT_ALLOC (pool, struct pool, dhcp_type_pool)
OMAPI_OBJECT_ALLOC (host, struct host_decl, dhcp_type_host)

struct lease_state *free_lease_states;

struct lease_state *new_lease_state (file, line)
	const char *file;
	int line;
{
	struct lease_state *rval;

	if (free_lease_states) {
		rval = free_lease_states;
		free_lease_states =
			(struct lease_state *)(free_lease_states -> next);
 		dmalloc_reuse (rval, file, line, 0);
	} else {
		rval = dmalloc (sizeof (struct lease_state), file, line);
		if (!rval)
			return rval;
	}
	memset (rval, 0, sizeof *rval);
	if (!option_state_allocate (&rval -> options, file, line)) {
		free_lease_state (rval, file, line);
		return (struct lease_state *)0;
	}
	return rval;
}

void free_lease_state (ptr, file, line)
	struct lease_state *ptr;
	const char *file;
	int line;
{
	if (ptr -> options)
		option_state_dereference (&ptr -> options, file, line);
	if (ptr -> packet)
		packet_dereference (&ptr -> packet, file, line);
	if (ptr -> shared_network)
		shared_network_dereference (&ptr -> shared_network,
					    file, line);

	data_string_forget (&ptr -> parameter_request_list, file, line);
	data_string_forget (&ptr -> filename, file, line);
	data_string_forget (&ptr -> server_name, file, line);
	ptr -> next = free_lease_states;
	free_lease_states = ptr;
	dmalloc_reuse (free_lease_states, (char *)0, 0, 0);
}

struct permit *new_permit (file, line)
	const char *file;
	int line;
{
	struct permit *permit = ((struct permit *)
				 dmalloc (sizeof (struct permit), file, line));
	if (!permit)
		return permit;
	memset (permit, 0, sizeof *permit);
	return permit;
}

void free_permit (permit, file, line)
	struct permit *permit;
	const char *file;
	int line;
{
	dfree (permit, file, line);
}
