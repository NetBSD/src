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

OMAPI_OBJECT_ALLOC (omapi_connection,
		    omapi_connection_object_t, omapi_type_connection)

isc_result_t omapi_connect (omapi_object_t *c,
			    const char *server_name,
			    unsigned port)
{
	struct hostent *he;
	unsigned i, hix;
	omapi_addr_list_t *addrs = (omapi_addr_list_t *)0;
	struct in_addr foo;
	isc_result_t status;

	if (!inet_aton (server_name, &foo)) {
		/* If we didn't get a numeric address, try for a domain
		   name.  It's okay for this call to block. */
		he = gethostbyname (server_name);
		if (!he)
			return ISC_R_HOSTUNKNOWN;
		for (i = 0; he -> h_addr_list [i]; i++)
			;
		if (i == 0)
			return ISC_R_HOSTUNKNOWN;
		hix = i;

		status = omapi_addr_list_new (&addrs, hix, MDL);
		if (status != ISC_R_SUCCESS)
			return status;
		for (i = 0; i < hix; i++) {
			addrs -> addresses [i].addrtype = he -> h_addrtype;
			addrs -> addresses [i].addrlen = he -> h_length;
			memcpy (addrs -> addresses [i].address,
				he -> h_addr_list [i],
				(unsigned)he -> h_length);
			addrs -> addresses [i].port = port;
		}
	} else {
		status = omapi_addr_list_new (&addrs, 1, MDL);
		if (status != ISC_R_SUCCESS)
			return status;
		addrs -> addresses [0].addrtype = AF_INET;
		addrs -> addresses [0].addrlen = sizeof foo;
		memcpy (addrs -> addresses [0].address, &foo, sizeof foo);
		addrs -> addresses [0].port = port;
		hix = 1;
	}
	status = omapi_connect_list (c, addrs, (omapi_addr_t *)0);
	omapi_addr_list_dereference (&addrs, MDL);
	return status;
}

isc_result_t omapi_connect_list (omapi_object_t *c,
				 omapi_addr_list_t *remote_addrs,
				 omapi_addr_t *local_addr)
{
	isc_result_t status;
	omapi_connection_object_t *obj;
	int flag;
	struct sockaddr_in local_sin;

	obj = (omapi_connection_object_t *)0;
	status = omapi_connection_allocate (&obj, MDL);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_object_reference (&c -> outer, (omapi_object_t *)obj,
					 MDL);
	if (status != ISC_R_SUCCESS) {
		omapi_connection_dereference (&obj, MDL);
		return status;
	}
	status = omapi_object_reference (&obj -> inner, c, MDL);
	if (status != ISC_R_SUCCESS) {
		omapi_connection_dereference (&obj, MDL);
		return status;
	}

	/* Create a socket on which to communicate. */
	obj -> socket =
		socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (obj -> socket < 0) {
		omapi_connection_dereference (&obj, MDL);
		if (errno == EMFILE || errno == ENFILE || errno == ENOBUFS)
			return ISC_R_NORESOURCES;
		return ISC_R_UNEXPECTED;
	}

	/* Set up the local address, if any. */
	if (local_addr) {
		/* Only do TCPv4 so far. */
		if (local_addr -> addrtype != AF_INET) {
			omapi_connection_dereference (&obj, MDL);
			return ISC_R_INVALIDARG;
		}
		local_sin.sin_port = htons (local_addr -> port);
		memcpy (&local_sin.sin_addr,
			local_addr -> address,
			local_addr -> addrlen);
#if defined (HAVE_SA_LEN)
		local_sin.sin_len = sizeof local_addr;
#endif
		local_sin.sin_family = AF_INET;
		memset (&local_sin.sin_zero, 0, sizeof local_sin.sin_zero);

		if (bind (obj -> socket, (struct sockaddr *)&local_sin,
			  sizeof local_sin) < 0) {
			omapi_object_dereference ((omapi_object_t **)&obj,
						  MDL);
			if (errno == EADDRINUSE)
				return ISC_R_ADDRINUSE;
			if (errno == EADDRNOTAVAIL)
				return ISC_R_ADDRNOTAVAIL;
			if (errno == EACCES)
				return ISC_R_NOPERM;
			return ISC_R_UNEXPECTED;
		}
	}

#if defined (HAVE_SETFD)
	if (fcntl (obj -> socket, F_SETFD, 1) < 0) {
		close (obj -> socket);
		omapi_connection_dereference (&obj, MDL);
		return ISC_R_UNEXPECTED;
	}
#endif

	/* Set the SO_REUSEADDR flag (this should not fail). */
	flag = 1;
	if (setsockopt (obj -> socket, SOL_SOCKET, SO_REUSEADDR,
			(char *)&flag, sizeof flag) < 0) {
		omapi_connection_dereference (&obj, MDL);
		return ISC_R_UNEXPECTED;
	}
	
	/* Set the file to nonblocking mode. */
	if (fcntl (obj -> socket, F_SETFL, O_NONBLOCK) < 0) {
		omapi_connection_dereference (&obj, MDL);
		return ISC_R_UNEXPECTED;
	}

	/* Store the address list on the object. */
	omapi_addr_list_reference (&obj -> connect_list, remote_addrs, MDL);
	obj -> cptr = 0;

	status = (omapi_register_io_object
		  ((omapi_object_t *)obj,
		   0, omapi_connection_writefd,
		   0, omapi_connection_connect,
		   omapi_connection_reaper));

	obj -> state = omapi_connection_unconnected;
	omapi_connection_connect ((omapi_object_t *)obj);
	omapi_connection_dereference (&obj, MDL);
	return status;
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
	if (h -> outer) {
		if (h -> outer -> inner)
			omapi_object_dereference (&h -> outer -> inner, MDL);
		omapi_object_dereference (&h -> outer, MDL);
	}

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
	if (c -> state == omapi_connection_connecting)
		return c -> socket;
	if (c -> out_bytes)
		return c -> socket;
	else
		return -1;
}

isc_result_t omapi_connection_connect (omapi_object_t *h)
{
	int error;
	omapi_connection_object_t *c;
	SOCKLEN_T sl;
	isc_result_t status;

	if (h -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;
	c = (omapi_connection_object_t *)h;

	if (c -> state == omapi_connection_connecting) {
		sl = sizeof error;
		if (getsockopt (c -> socket, SOL_SOCKET, SO_ERROR,
				(char *)&error, &sl) < 0) {
			omapi_disconnect (h, 1);
			return ISC_R_SUCCESS;
		}
		if (!error)
			c -> state = omapi_connection_connected;
	}
	if (c -> state == omapi_connection_connecting ||
	    c -> state == omapi_connection_unconnected) {
		if (c -> cptr >= c -> connect_list -> count) {
			omapi_disconnect (h, 1);
			return ISC_R_SUCCESS;
		}

		if (c -> connect_list -> addresses [c -> cptr].addrtype !=
		    AF_INET) {
			omapi_disconnect (h, 1);
			return ISC_R_SUCCESS;
		}

		memcpy (&c -> remote_addr.sin_addr,
			&c -> connect_list -> addresses [c -> cptr].address,
			sizeof c -> remote_addr.sin_addr);
		c -> remote_addr.sin_family = AF_INET;
		c -> remote_addr.sin_port =
		       htons (c -> connect_list -> addresses [c -> cptr].port);
#if defined (HAVE_SA_LEN)
		c -> remote_addr.sin_len = sizeof c -> remote_addr;
#endif
		memset (&c -> remote_addr.sin_zero, 0,
			sizeof c -> remote_addr.sin_zero);
		++c -> cptr;

		error = connect (c -> socket,
				 (struct sockaddr *)&c -> remote_addr,
				 sizeof c -> remote_addr);
		if (error < 0) {
			error = errno;
			if (error != EINPROGRESS) {
				omapi_disconnect (h, 1);
				return ISC_R_UNEXPECTED;
			}
			c -> state = omapi_connection_connecting;
			return ISC_R_SUCCESS;
		}
		c -> state = omapi_connection_connected;
	}
	
	/* I don't know why this would fail, so I'm tempted not to test
	   the return value. */
	sl = sizeof (c -> local_addr);
	if (getsockname (c -> socket,
			 (struct sockaddr *)&c -> local_addr, &sl) < 0) {
	}

	/* Disconnect from I/O object, if any. */
	if (h -> outer)
		omapi_unregister_io_object (h);

	status = omapi_register_io_object (h,
					   omapi_connection_readfd,
					   omapi_connection_writefd,
					   omapi_connection_reader,
					   omapi_connection_writer,
					   omapi_connection_reaper);

	if (status != ISC_R_SUCCESS) {
		omapi_disconnect (h, 1);
		return status;
	}

	omapi_signal_in (h, "connect");
	omapi_addr_list_dereference (&c -> connect_list, MDL);
	return ISC_R_SUCCESS;
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
		omapi_listener_dereference (&c -> listener, file, line);
	if (c -> connect_list)
		omapi_addr_list_dereference (&c -> connect_list, file, line);
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
