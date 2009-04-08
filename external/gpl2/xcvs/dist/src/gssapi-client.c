/* CVS GSSAPI client stuff.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#include "cvs.h"
#include "buffer.h"

#if defined (CLIENT_SUPPORT) || defined (SERVER_SUPPORT)
# include "gssapi-client.h"

/* This is needed for GSSAPI encryption.  */
gss_ctx_id_t gcontext;
#endif /* CLIENT_SUPPORT || SERVER_SUPPORT */

#ifdef CLIENT_SUPPORT
# include "socket-client.h"

# ifdef ENCRYPTION
/* Whether to encrypt GSSAPI communication.  We use a global variable
   like this because we use the same buffer type (gssapi_wrap) to
   handle both authentication and encryption, and we don't want
   multiple instances of that buffer in the communication stream.  */
int cvs_gssapi_encrypt;
# endif /* ENCRYPTION */


/* Receive a given number of bytes.  */

static void
recv_bytes (int sock, char *buf, int need)
{
    while (need > 0)
    {
	int got;

	got = recv (sock, buf, need, 0);
	if (got <= 0)
	    error (1, 0, "recv() from server %s: %s",
                   current_parsed_root->hostname,
		   got == 0 ? "EOF" : SOCK_STRERROR (SOCK_ERRNO));

	buf += got;
	need -= got;
    }
}



/* Connect to the server using GSSAPI authentication.  */

/* FIXME
 *
 * This really needs to be rewritten to use a buffer and not a socket.
 * This would enable gserver to work with the SSL code I'm about to commit
 * since the SSL connection is going to look like a FIFO and not a socket.
 *
 * I think, basically, it will need to use buf_output and buf_read directly
 * since I don't think there is a read_bytes function - only read_line.
 *
 * recv_bytes could then be removed too.
 *
 * Besides, I added some cruft to reenable the socket which shouldn't be
 * there.  This would also enable its removal.
 */
#define BUFSIZE 1024
int
connect_to_gserver (cvsroot_t *root, int sock, const char *hostname)
{
    char *str;
    char buf[BUFSIZE];
    gss_buffer_desc *tok_in_ptr, tok_in, tok_out;
    OM_uint32 stat_min, stat_maj;
    gss_name_t server_name;

    str = "BEGIN GSSAPI REQUEST\012";

    if (send (sock, str, strlen (str), 0) < 0)
	error (1, 0, "cannot send: %s", SOCK_STRERROR (SOCK_ERRNO));

    if (strlen (hostname) > BUFSIZE - 5)
	error (1, 0, "Internal error: hostname exceeds length of buffer");
    snprintf (buf, sizeof(buf), "cvs@%s", hostname);
    tok_in.length = strlen (buf);
    tok_in.value = buf;
    gss_import_name (&stat_min, &tok_in, GSS_C_NT_HOSTBASED_SERVICE,
		     &server_name);

    tok_in_ptr = GSS_C_NO_BUFFER;
    gcontext = GSS_C_NO_CONTEXT;

    do
    {
	stat_maj = gss_init_sec_context (&stat_min, GSS_C_NO_CREDENTIAL,
					 &gcontext, server_name,
					 GSS_C_NULL_OID,
					 (GSS_C_MUTUAL_FLAG
					  | GSS_C_REPLAY_FLAG),
					 0, NULL, tok_in_ptr, NULL, &tok_out,
					 NULL, NULL);
	if (stat_maj != GSS_S_COMPLETE && stat_maj != GSS_S_CONTINUE_NEEDED)
	{
	    OM_uint32 message_context;
	    OM_uint32 new_stat_min;

	    message_context = 0;
	    gss_display_status (&new_stat_min, stat_maj, GSS_C_GSS_CODE,
                                GSS_C_NULL_OID, &message_context, &tok_out);
	    error (0, 0, "GSSAPI authentication failed: %s",
		   (const char *) tok_out.value);

	    message_context = 0;
	    gss_display_status (&new_stat_min, stat_min, GSS_C_MECH_CODE,
				GSS_C_NULL_OID, &message_context, &tok_out);
	    error (1, 0, "GSSAPI authentication failed: %s",
		   (const char *) tok_out.value);
	}

	if (tok_out.length == 0)
	{
	    tok_in.length = 0;
	}
	else
	{
	    char cbuf[2];
	    int need;

	    cbuf[0] = (tok_out.length >> 8) & 0xff;
	    cbuf[1] = tok_out.length & 0xff;
	    if (send (sock, cbuf, 2, 0) < 0)
		error (1, 0, "cannot send: %s", SOCK_STRERROR (SOCK_ERRNO));
	    if (send (sock, tok_out.value, tok_out.length, 0) < 0)
		error (1, 0, "cannot send: %s", SOCK_STRERROR (SOCK_ERRNO));

	    recv_bytes (sock, cbuf, 2);
	    need = ((cbuf[0] & 0xff) << 8) | (cbuf[1] & 0xff);

	    if (need > sizeof buf)
	    {
		ssize_t got;
		size_t total;

		/* This usually means that the server sent us an error
		   message.  Read it byte by byte and print it out.
		   FIXME: This is a terrible error handling strategy.
		   However, even if we fix the server, we will still
		   want to do this to work with older servers.  */
		buf[0] = cbuf[0];
		buf[1] = cbuf[1];
		total = 2;
		while ((got = recv (sock, buf + total, sizeof buf - total, 0)))
		{
		    if (got < 0)
			error (1, 0, "recv() from server %s: %s",
			       root->hostname, SOCK_STRERROR (SOCK_ERRNO));
		    total += got;
		    if (strrchr (buf + total - got, '\n'))
			break;
		}
		buf[total] = '\0';
		if (buf[total - 1] == '\n')
		    buf[total - 1] = '\0';
		error (1, 0, "error from server %s: %s", root->hostname,
		       buf);
	    }

	    recv_bytes (sock, buf, need);
	    tok_in.length = need;
	}

	tok_in.value = buf;
	tok_in_ptr = &tok_in;
    }
    while (stat_maj == GSS_S_CONTINUE_NEEDED);

    return 1;
}
#endif /* CLIENT_SUPPORT */



#if defined (CLIENT_SUPPORT) || defined (SERVER_SUPPORT)
/* A buffer interface using GSSAPI.  It is built on top of a
   packetizing buffer.  */

/* This structure is the closure field of the GSSAPI translation
   routines.  */

struct cvs_gssapi_wrap_data
{
    /* The GSSAPI context.  */
    gss_ctx_id_t gcontext;
};



/* Unwrap data using GSSAPI.  */
static int
cvs_gssapi_wrap_input (void *fnclosure, const char *input, char *output,
                       size_t size)
{
    struct cvs_gssapi_wrap_data *gd = fnclosure;
    gss_buffer_desc inbuf, outbuf;
    OM_uint32 stat_min;
    int conf;

    inbuf.value = (void *)input;
    inbuf.length = size;

    if (gss_unwrap (&stat_min, gd->gcontext, &inbuf, &outbuf, &conf, NULL)
	!= GSS_S_COMPLETE)
    {
	error (1, 0, "gss_unwrap failed");
    }

    if (outbuf.length > size)
	abort ();

    memcpy (output, outbuf.value, outbuf.length);

    /* The real packet size is stored in the data, so we don't need to
       remember outbuf.length.  */

    gss_release_buffer (&stat_min, &outbuf);

    return 0;
}



/* Wrap data using GSSAPI.  */
static int
cvs_gssapi_wrap_output (void *fnclosure, const char *input, char *output,
                        size_t size, size_t *translated)
{
    struct cvs_gssapi_wrap_data *gd = fnclosure;
    gss_buffer_desc inbuf, outbuf;
    OM_uint32 stat_min;
    int conf_req, conf;

    inbuf.value = (void *)input;
    inbuf.length = size;

#ifdef ENCRYPTION
    conf_req = cvs_gssapi_encrypt;
#else
    conf_req = 0;
#endif

    if (gss_wrap (&stat_min, gd->gcontext, conf_req, GSS_C_QOP_DEFAULT,
		  &inbuf, &conf, &outbuf) != GSS_S_COMPLETE)
	error (1, 0, "gss_wrap failed");

    /* The packetizing buffer only permits us to add 100 bytes.
       FIXME: I don't know what, if anything, is guaranteed by GSSAPI.
       This may need to be increased for a different GSSAPI
       implementation, or we may need a different algorithm.  */
    if (outbuf.length > size + 100)
	abort ();

    memcpy (output, outbuf.value, outbuf.length);

    *translated = outbuf.length;

    gss_release_buffer (&stat_min, &outbuf);

    return 0;
}



/* Create a GSSAPI wrapping buffer.  We use a packetizing buffer with
   GSSAPI wrapping routines.  */
struct buffer *
cvs_gssapi_wrap_buffer_initialize (struct buffer *buf, int input,
                                   gss_ctx_id_t gcontext,
                                   void (*memory) ( struct buffer * ))
{
    struct cvs_gssapi_wrap_data *gd;

    gd = xmalloc (sizeof *gd);
    gd->gcontext = gcontext;

    return packetizing_buffer_initialize (buf,
                                          input ? cvs_gssapi_wrap_input
						: NULL,
                                          input ? NULL
						: cvs_gssapi_wrap_output,
                                          gd, memory);
}



void
initialize_gssapi_buffers (struct buffer **to_server_p,
                           struct buffer **from_server_p)
{
  *to_server_p = cvs_gssapi_wrap_buffer_initialize (*to_server_p, 0,
						    gcontext, NULL);

  *from_server_p = cvs_gssapi_wrap_buffer_initialize (*from_server_p, 1,
						      gcontext, NULL);
}
#endif /* CLIENT_SUPPORT || SERVER_SUPPORT */
