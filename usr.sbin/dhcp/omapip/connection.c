/* connection.c

   Subroutines for dealing with connections. */

/*
 * Copyright (c) 1999-2000 Internet Software Consortium.
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

#include <omapip/omapip_p.h>
#include <arpa/inet.h>

isc_result_t omapi_connect (omapi_object_t *c,
			    const char *server_name,
			    unsigned port)
{
	struct hostent *he;
	int hix;
	isc_result_t status;
	omapi_connection_object_t *obj;
	int flag;
	SOCKLEN_T sl;

	obj = (omapi_connection_object_t *)dmalloc (sizeof *obj, MDL);
	if (!obj)
		return ISC_R_NOMEMORY;
	memset (obj, 0, sizeof *obj);
	obj -> refcnt = 1;
	rc_register_mdl (&obj, obj, obj -> refcnt);
	obj -> type = omapi_type_connection;

	status = omapi_object_reference (&c -> outer, (omapi_object_t *)obj,
					 MDL);
	if (status != ISC_R_SUCCESS) {
		omapi_object_dereference ((omapi_object_t **)&obj, MDL);
		return status;
	}
	status = omapi_object_reference (&obj -> inner, c, MDL);
	if (status != ISC_R_SUCCESS) {
		omapi_object_dereference ((omapi_object_t **)&obj, MDL);
		return status;
	}

	/* Set up all the constants in the address... */
	obj -> remote_addr.sin_port = htons (port);

	/* First try for a numeric address, since that's easier to check. */
	if (!inet_aton (server_name, &obj -> remote_addr.sin_addr)) {
		/* If we didn't get a numeric address, try for a domain
		   name.  It's okay for this call to block. */
		he = gethostbyname (server_name);
		if (!he) {
			omapi_object_dereference ((omapi_object_t **)&obj,
						  MDL);
			return ISC_R_HOSTUNKNOWN;
		}
		hix = 1;
		memcpy (&obj -> remote_addr.sin_addr,
			he -> h_addr_list [0],
			sizeof obj -> remote_addr.sin_addr);
	} else
		he = (struct hostent *)0;

#if defined (HAVE_SA_LEN)
	obj -> remote_addr.sin_len =
		sizeof (struct sockaddr_in);
#endif
	obj -> remote_addr.sin_family = AF_INET;
	memset (&(obj -> remote_addr.sin_zero), 0,
		sizeof obj -> remote_addr.sin_zero);

	/* Create a socket on which to communicate. */
	obj -> socket =
		socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (obj -> socket < 0) {
		omapi_object_dereference ((omapi_object_t **)&obj, MDL);
		if (errno == EMFILE || errno == ENFILE || errno == ENOBUFS)
			return ISC_R_NORESOURCES;
		return ISC_R_UNEXPECTED;
	}

#if defined (HAVE_SETFD)
	if (fcntl (obj -> socket, F_SETFD, 1) < 0) {
		close (obj -> socket);
		omapi_object_dereference ((omapi_object_t **)&obj, MDL);
		return ISC_R_UNEXPECTED;
	}
#endif

	/* Set the SO_REUSEADDR flag (this should not fail). */
	flag = 1;
	if (setsockopt (obj -> socket, SOL_SOCKET, SO_REUSEADDR,
			(char *)&flag, sizeof flag) < 0) {
		omapi_object_dereference ((omapi_object_t **)&obj, MDL);
		return ISC_R_UNEXPECTED;
	}
	
	/* Try to connect to the one IP address we were given, or any of
	   the IP addresses listed in the host's A RR. */
	while (connect (obj -> socket,
			((struct sockaddr *)
			 &obj -> remote_addr),
			sizeof obj -> remote_addr)) {
		if (!he || !he -> h_addr_list [hix]) {
			omapi_object_dereference ((omapi_object_t **)&obj,
						  MDL);
			if (errno == ECONNREFUSED)
				return ISC_R_CONNREFUSED;
			if (errno == ENETUNREACH)
				return ISC_R_NETUNREACH;
			return ISC_R_UNEXPECTED;
		}
		memcpy (&obj -> remote_addr.sin_addr,
			he -> h_addr_list [hix++],
			sizeof obj -> remote_addr.sin_addr);
	}

	obj -> state = omapi_connection_connected;

	/* I don't know why this would fail, so I'm tempted not to test
	   the return value. */
	sl = sizeof (obj -> local_addr);
	if (getsockname (obj -> socket,
			 ((struct sockaddr *)
			  &obj -> local_addr), &sl) < 0) {
	}

	if (fcntl (obj -> socket, F_SETFL, O_NONBLOCK) < 0) {
		omapi_object_dereference ((omapi_object_t **)&obj, MDL);
		return ISC_R_UNEXPECTED;
	}

	status = omapi_register_io_object ((omapi_object_t *)obj,
					   omapi_connection_readfd,
					   omapi_connection_writefd,
					   omapi_connection_reader,
					   omapi_connection_writer,
					   omapi_connection_reaper);
	if (status != ISC_R_SUCCESS) {
		omapi_object_dereference ((omapi_object_t **)&obj, MDL);
		return status;
	}

	return ISC_R_SUCCESS;
}

/* Disconnect a connection object from the remote end.   If force is nonzero,
   close the connection immediately.   Otherwise, shut down the receiving end
   but allow any unsent data to be sent before actually closing the socket. */

isc_result_t omapi_disconnect (omapi_object_t *h,
			       int force)
{
	omapi_connection_object_t *c;

	c = (omapi_connection_object_t *)h;
	if (c -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;

	if (!force) {
		/* If we're already disconnecting, we don't have to do
		   anything. */
		if (c -> state == omapi_connection_disconnecting)
			return ISC_R_SUCCESS;

		/* Try to shut down the socket - this sends a FIN to the
		   remote end, so that it won't send us any more data.   If
		   the shutdown succeeds, and we still have bytes left to
		   write, defer closing the socket until that's done. */
		if (!shutdown (c -> socket, SHUT_RD)) {
			if (c -> out_bytes > 0) {
				c -> state = omapi_connection_disconnecting;
				return ISC_R_SUCCESS;
			}
		}
	}
	close (c -> socket);
	c -> state = omapi_connection_closed;

	/* Disconnect from I/O object, if any. */
	if (h -> outer)
		omapi_object_dereference (&h -> outer, MDL);

	/* If whatever created us registered a signal handler, send it
	   a disconnect signal. */
	omapi_signal (h, "disconnect", h);
	return ISC_R_SUCCESS;
}

isc_result_t omapi_connection_require (omapi_object_t *h, unsigned bytes)
{
	omapi_connection_object_t *c;

	if (h -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;
	c = (omapi_connection_object_t *)h;

	c -> bytes_needed = bytes;
	if (c -> bytes_needed <= c -> in_bytes) {
		return ISC_R_SUCCESS;
	}
	return ISC_R_NOTYET;
}

/* Return the socket on which the dispatcher should wait for readiness
   to read, for a connection object.   If we already have more bytes than
   we need to do the next thing, and we have at least a single full input
   buffer, then don't indicate that we're ready to read. */
int omapi_connection_readfd (omapi_object_t *h)
{
	omapi_connection_object_t *c;
	if (h -> type != omapi_type_connection)
		return -1;
	c = (omapi_connection_object_t *)h;
	if (c -> state != omapi_connection_connected)
		return -1;
	if (c -> in_bytes >= OMAPI_BUF_SIZE - 1 &&
	    c -> in_bytes > c -> bytes_needed)
		return -1;
	return c -> socket;
}

/* Return the socket on which the dispatcher should wait for readiness
   to write, for a connection object.   If there are no bytes buffered
   for writing, then don't indicate that we're ready to write. */
int omapi_connection_writefd (omapi_object_t *h)
{
	omapi_connection_object_t *c;
	if (h -> type != omapi_type_connection)
		return -1;
	c = (omapi_connection_object_t *)h;
	if (c -> out_bytes)
		return c -> socket;
	else
		return -1;
}

/* Reaper function for connection - if the connection is completely closed,
   reap it.   If it's in the disconnecting state, there were bytes left
   to write when the user closed it, so if there are now no bytes left to
   write, we can close it. */
isc_result_t omapi_connection_reaper (omapi_object_t *h)
{
	omapi_connection_object_t *c;

	if (h -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;

	c = (omapi_connection_object_t *)h;
	if (c -> state == omapi_connection_disconnecting &&
	    c -> out_bytes == 0)
		omapi_disconnect (h, 1);
	if (c -> state == omapi_connection_closed)
		return ISC_R_NOTCONNECTED;
	return ISC_R_SUCCESS;
}

isc_result_t omapi_connection_set_value (omapi_object_t *h,
					 omapi_object_t *id,
					 omapi_data_string_t *name,
					 omapi_typed_data_t *value)
{
	if (h -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;
	
	if (h -> inner && h -> inner -> type -> set_value)
		return (*(h -> inner -> type -> set_value))
			(h -> inner, id, name, value);
	return ISC_R_NOTFOUND;
}

isc_result_t omapi_connection_get_value (omapi_object_t *h,
					 omapi_object_t *id,
					 omapi_data_string_t *name,
					 omapi_value_t **value)
{
	if (h -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;
	
	if (h -> inner && h -> inner -> type -> get_value)
		return (*(h -> inner -> type -> get_value))
			(h -> inner, id, name, value);
	return ISC_R_NOTFOUND;
}

isc_result_t omapi_connection_destroy (omapi_object_t *h,
				       const char *file, int line)
{
	omapi_connection_object_t *c;

	if (h -> type != omapi_type_connection)
		return ISC_R_UNEXPECTED;
	c = (omapi_connection_object_t *)(h);
	if (c -> state == omapi_connection_connected)
		omapi_disconnect (h, 1);
	if (c -> listener)
		omapi_object_dereference (&c -> listener, file, line);
	return ISC_R_SUCCESS;
}

isc_result_t omapi_connection_signal_handler (omapi_object_t *h,
					      const char *name, va_list ap)
{
	if (h -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;
	
	if (h -> inner && h -> inner -> type -> signal_handler)
		return (*(h -> inner -> type -> signal_handler)) (h -> inner,
								  name, ap);
	return ISC_R_NOTFOUND;
}

/* Write all the published values associated with the object through the
   specified connection. */

isc_result_t omapi_connection_stuff_values (omapi_object_t *c,
					    omapi_object_t *id,
					    omapi_object_t *m)
{
	int i;

	if (m -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;

	if (m -> inner && m -> inner -> type -> stuff_values)
		return (*(m -> inner -> type -> stuff_values)) (c, id,
								m -> inner);
	return ISC_R_SUCCESS;
}
