/*	$NetBSD: unix_dgram_connect.c,v 1.3 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	unix_dgram_connect 3
/* SUMMARY
/*	connect to UNIX-domain datagram server
/* SYNOPSIS
/*	#include <connect.h>
/*
/*	int	unix_dgram_connect(
/*	const char *path,
/*	int	block_mode)
/* DESCRIPTION
/*	unix_dgram_connect() connects to the specified UNIX-domain
/*	datagram server, and returns the resulting file descriptor.
/*
/*	Arguments:
/* .IP path
/*	Null-terminated string with connection destination.`
/* .IP block_mode
/*	Either NON_BLOCKING for a non-blocking socket, or BLOCKING for
/*	blocking mode.
/* DIAGNOSTICS
/*	Fatal errors: path too large, can't create socket.
/*
/*	Other errors result in a -1 result value, with errno indicating
/*	why the service is unavailable.
/* .sp
/*	ENOENT: the named socket does not exist.
/* .sp
/*	ECONNREFUSED: the named socket is not open.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <connect.h>
#include <iostuff.h>

/* unix_dgram_connect - connect to UNIX-domain datagram service */

int     unix_dgram_connect(const char *path, int block_mode)
{
    const char myname[] = "unix_dgram_connect";
#undef sun
    struct sockaddr_un sun;
    ssize_t path_len;
    int     sock;

    /*
     * Translate address information to internal form.
     */
    if ((path_len = strlen(path)) >= sizeof(sun.sun_path))
	msg_fatal("%s: unix-domain name too long: %s", myname, path);
    memset((void *) &sun, 0, sizeof(sun));
    sun.sun_family = AF_UNIX;
#ifdef HAS_SUN_LEN
    sun.sun_len = path_len + 1;
#endif
    memcpy(sun.sun_path, path, path_len + 1);

    /*
     * Create a client socket.
     */
    if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
	msg_fatal("%s: socket: %m", myname);
    if (connect(sock, (struct sockaddr *) &sun, sizeof(sun)) < 0) {
	close(sock);
	return (-1);
    }
    non_blocking(sock, block_mode);
    return (sock);
}
