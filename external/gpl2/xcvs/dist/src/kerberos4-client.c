/* CVS Kerberos4 client stuff.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#include <config.h>

#include "cvs.h"

#include "buffer.h"
#include "socket-client.h"

#   include <krb.h>

extern char *krb_realmofhost ();
#   ifndef HAVE_KRB_GET_ERR_TEXT
#     define krb_get_err_text(status) krb_err_txt[status]
#   endif /* HAVE_KRB_GET_ERR_TEXT */

/* Information we need if we are going to use Kerberos encryption.  */
static C_Block kblock;
static Key_schedule sched;


/* This function has not been changed to deal with NO_SOCKET_TO_FD
   (i.e., systems on which sockets cannot be converted to file
   descriptors).  The first person to try building a kerberos client
   on such a system (OS/2, Windows 95, and maybe others) will have to
   take care of this.  */
void
start_kerberos4_server (cvsroot_t *root, struct buffer **to_server_p,
                        struct buffer **from_server_p)
{
    int s;
    int port;
    struct hostent *hp;
    struct sockaddr_in sin;
    char *hname;

    s = socket (AF_INET, SOCK_STREAM, 0);
    if (s < 0)
	error (1, 0, "cannot create socket: %s", SOCK_STRERROR (SOCK_ERRNO));

    port = get_cvs_port_number (root);

    hp = init_sockaddr (&sin, root->hostname, port);

    hname = xstrdup (hp->h_name);
  
    TRACE (TRACE_FUNCTION, "Connecting to %s(%s):%d",
	   root->hostname,
	   inet_ntoa (sin.sin_addr),
	   port);

    if (connect (s, (struct sockaddr *) &sin, sizeof sin) < 0)
	error (1, 0, "connect to %s(%s):%d failed: %s",
	       root->hostname,
	       inet_ntoa (sin.sin_addr),
	       port, SOCK_STRERROR (SOCK_ERRNO));

    {
	const char *realm;
	struct sockaddr_in laddr;
	int laddrlen;
	KTEXT_ST ticket;
	MSG_DAT msg_data;
	CREDENTIALS cred;
	int status;

	realm = krb_realmofhost (hname);

	laddrlen = sizeof (laddr);
	if (getsockname (s, (struct sockaddr *) &laddr, &laddrlen) < 0)
	    error (1, 0, "getsockname failed: %s", SOCK_STRERROR (SOCK_ERRNO));

	/* We don't care about the checksum, and pass it as zero.  */
	status = krb_sendauth (KOPT_DO_MUTUAL, s, &ticket, "rcmd",
			       hname, realm, (unsigned long) 0, &msg_data,
			       &cred, sched, &laddr, &sin, "KCVSV1.0");
	if (status != KSUCCESS)
	    error (1, 0, "kerberos authentication failed: %s",
		   krb_get_err_text (status));
	memcpy (kblock, cred.session, sizeof (C_Block));
    }

    close_on_exec (s);

    free (hname);

    /* Give caller the values it wants. */
    make_bufs_from_fds (s, s, 0, root, to_server_p, from_server_p, 1);
}

void
initialize_kerberos4_encryption_buffers( struct buffer **to_server_p,
                                         struct buffer **from_server_p )
{
  *to_server_p = krb_encrypt_buffer_initialize (*to_server_p, 0, sched,
						kblock, NULL);
  *from_server_p = krb_encrypt_buffer_initialize (*from_server_p, 1,
						  sched, kblock, NULL);
}

