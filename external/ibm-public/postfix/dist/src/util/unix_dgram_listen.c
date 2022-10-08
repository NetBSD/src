/*	$NetBSD: unix_dgram_listen.c,v 1.3 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	unix_dgram_listen 3
/* SUMMARY
/*	listen to UNIX-domain datagram server
/* SYNOPSIS
/*	#include <listen.h>
/*
/*	int	unix_dgram_listen(
/*	const char *path,
/*	int	block_mode)
/* DESCRIPTION
/*	unix_dgram_listen() binds to the specified UNIX-domain
/*	datagram endpoint, and returns the resulting file descriptor.
/*
/*	Arguments:
/* .IP path
/*	Null-terminated string with connection destination.
/* .IP backlog
/*	Either NON_BLOCKING for a non-blocking socket, or BLOCKING for
/*	blocking mode.
/* DIAGNOSTICS
/*	Fatal errors: path too large, can't create socket.
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
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

 /*
  * Utility library.
  */
#include <iostuff.h>
#include <listen.h>
#include <msg.h>

/* unix_dgram_listen - bind to UNIX-domain datagram endpoint */

int     unix_dgram_listen(const char *path, int block_mode)
{
    const char myname[] = "unix_dgram_listen";
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
     * Create a 'server' socket.
     */
    if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
	msg_fatal("%s: socket: %m", myname);
    if (unlink(path) < 0 && errno != ENOENT) 
        msg_fatal( "remove %s: %m", path);
    if (bind(sock, (struct sockaddr *) & sun, sizeof(sun)) < 0) 
        msg_fatal( "bind: %s: %m", path);
#ifdef FCHMOD_UNIX_SOCKETS
    if (fchmod(sock, 0666) < 0)
        msg_fatal("fchmod socket %s: %m", path);
#else
    if (chmod(path, 0666) < 0)
        msg_fatal("chmod socket %s: %m", path);
#endif
    non_blocking(sock, block_mode);
    return (sock);
}
