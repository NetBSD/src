/* CVS socket client stuff.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

/***
 *** THIS FILE SHOULD NEVER BE COMPILED UNLESS NO_SOCKET_TO_FD IS DEFINED.
 ***/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef CLIENT_SUPPORT

#include "cvs.h"
#include "buffer.h"

#include "socket-client.h"


/* Under certain circumstances, we must communicate with the server
   via a socket using send() and recv().  This is because under some
   operating systems (OS/2 and Windows 95 come to mind), a socket
   cannot be converted to a file descriptor -- it must be treated as a
   socket and nothing else.
   
   We may also need to deal with socket routine error codes differently
   in these cases.  This is handled through the SOCK_ERRNO and
   SOCK_STRERROR macros. */

/* These routines implement a buffer structure which uses send and
   recv.  The buffer is always in blocking mode so we don't implement
   the block routine.  */

/* Note that it is important that these routines always handle errors
   internally and never return a positive errno code, since it would in
   general be impossible for the caller to know in general whether any
   error code came from a socket routine (to decide whether to use
   SOCK_STRERROR or simply strerror to print an error message). */

/* We use an instance of this structure as the closure field.  */

struct socket_buffer
{
    /* The socket number.  */
    int socket;
};



/* The buffer input function for a buffer built on a socket.  */

static int
socket_buffer_input (void *closure, char *data, size_t need, size_t size,
		     size_t *got)
{
    struct socket_buffer *sb = closure;
    int nbytes;

    /* I believe that the recv function gives us exactly the semantics
       we want.  If there is a message, it returns immediately with
       whatever it could get.  If there is no message, it waits until
       one comes in.  In other words, it is not like read, which in
       blocking mode normally waits until all the requested data is
       available.  */

    assert (size >= need);

    *got = 0;

    do
    {

	/* Note that for certain (broken?) networking stacks, like
	   VMS's UCX (not sure what version, problem reported with
	   recv() in 1997), and (according to windows-NT/config.h)
	   Windows NT 3.51, we must call recv or send with a
	   moderately sized buffer (say, less than 200K or something),
	   or else there may be network errors (somewhat hard to
	   produce, e.g. WAN not LAN or some such).  buf_read_data
	   makes sure that we only recv() BUFFER_DATA_SIZE bytes at
	   a time.  */

	nbytes = recv (sb->socket, data + *got, size - *got, 0);
	if (nbytes < 0)
	    error (1, 0, "reading from server: %s",
		   SOCK_STRERROR (SOCK_ERRNO));
	if (nbytes == 0)
	{
	    /* End of file (for example, the server has closed
	       the connection).  If we've already read something, we
	       just tell the caller about the data, not about the end of
	       file.  If we've read nothing, we return end of file.  */
	    if (*got == 0)
		return -1;
	    else
		return 0;
	}
	*got += nbytes;
    }
    while (*got < need);

    return 0;
}



/* The buffer output function for a buffer built on a socket.  */

static int
socket_buffer_output (void *closure, const char *data, size_t have,
		      size_t *wrote)
{
    struct socket_buffer *sb = closure;

    *wrote = have;

    /* See comment in socket_buffer_input regarding buffer size we pass
       to send and recv.  */

# ifdef SEND_NEVER_PARTIAL
    /* If send() never will produce a partial write, then just do it.  This
       is needed for systems where its return value is something other than
       the number of bytes written.  */
    if (send (sb->socket, data, have, 0) < 0)
	error (1, 0, "writing to server socket: %s",
	       SOCK_STRERROR (SOCK_ERRNO));
# else
    while (have > 0)
    {
	int nbytes;

	nbytes = send (sb->socket, data, have, 0);
	if (nbytes < 0)
	    error (1, 0, "writing to server socket: %s",
		   SOCK_STRERROR (SOCK_ERRNO));

	have -= nbytes;
	data += nbytes;
    }
# endif

    return 0;
}



/* The buffer flush function for a buffer built on a socket.  */

/*ARGSUSED*/
static int
socket_buffer_flush (void *closure)
{
    /* Nothing to do.  Sockets are always flushed.  */
    return 0;
}



static int
socket_buffer_shutdown (struct buffer *buf)
{
    struct socket_buffer *n = buf->closure;
    char tmp;

    /* no need to flush children of an endpoint buffer here */

    if (buf->input)
    {
	int err = 0;
	if (! buf_empty_p (buf)
	    || (err = recv (n->socket, &tmp, 1, 0)) > 0)
	    error (0, 0, "dying gasps from %s unexpected",
		   current_parsed_root->hostname);
	else if (err == -1)
	    error (0, 0, "reading from %s: %s", current_parsed_root->hostname,
		   SOCK_STRERROR (SOCK_ERRNO));

	/* shutdown() socket */
# ifdef SHUTDOWN_SERVER
	if (current_parsed_root->method != server_method)
# endif
	if (shutdown (n->socket, 0) < 0)
	{
	    error (1, 0, "shutting down server socket: %s",
		   SOCK_STRERROR (SOCK_ERRNO));
	}

	buf->input = NULL;
    }
    else if (buf->output)
    {
	/* shutdown() socket */
# ifdef SHUTDOWN_SERVER
	/* FIXME:  Should have a SHUTDOWN_SERVER_INPUT &
	 * SHUTDOWN_SERVER_OUTPUT
	 */
	if (current_parsed_root->method == server_method)
	    SHUTDOWN_SERVER (n->socket);
	else
# endif
	if (shutdown (n->socket, 1) < 0)
	{
	    error (1, 0, "shutting down server socket: %s",
		   SOCK_STRERROR (SOCK_ERRNO));
	}

	buf->output = NULL;
    }

    return 0;
}



/* Create a buffer based on a socket.  */

struct buffer *
socket_buffer_initialize (int socket, int input,
                          void (*memory) (struct buffer *))
{
    struct socket_buffer *sbuf = xmalloc (sizeof *sbuf);
    sbuf->socket = socket;
    return buf_initialize (input ? socket_buffer_input : NULL,
			   input ? NULL : socket_buffer_output,
			   input ? NULL : socket_buffer_flush,
			   NULL, NULL,
			   socket_buffer_shutdown,
			   memory,
			   sbuf);
}

#endif /* CLIENT_SUPPORT */
