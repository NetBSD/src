/*	$NetBSD: master_listen.c,v 1.1.1.1 2009/06/23 10:08:49 tron Exp $	*/

/*++
/* NAME
/*	master_listen 3
/* SUMMARY
/*	Postfix master - start/stop listeners
/* SYNOPSIS
/*	#include "master.h"
/*
/*	void	master_listen_init(serv)
/*	MASTER_SERV *serv;
/*
/*	void	master_listen_cleanup(serv)
/*	MASTER_SERV *serv;
/* DESCRIPTION
/*	master_listen_init() turns on the listener implemented by the
/*	named process. FIFOs and UNIX-domain sockets are created with
/*	mode 0622 and with ownership mail_owner.
/*
/*	master_listen_cleanup() turns off the listener implemented by the
/*	named process.
/* DIAGNOSTICS
/* BUGS
/* SEE ALSO
/*	inet_listen(3), internet-domain listener
/*	unix_listen(3), unix-domain listener
/*	fifo_listen(3), named-pipe listener
/*	upass_listen(3), file descriptor passing listener
/*	set_eugid(3), set effective user/group attributes
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <listen.h>
#include <mymalloc.h>
#include <stringops.h>
#include <inet_addr_list.h>
#include <set_eugid.h>
#include <set_ugid.h>
#include <iostuff.h>
#include <myaddrinfo.h>
#include <sock_addr.h>

/* Global library. */

#include <mail_params.h>

/* Application-specific. */

#include "master.h"

/* master_listen_init - enable connection requests */

void    master_listen_init(MASTER_SERV *serv)
{
    const char *myname = "master_listen_init";
    char   *end_point;
    int     n;
    MAI_HOSTADDR_STR hostaddr;
    struct sockaddr *sa;

    /*
     * Find out what transport we should use, then create one or more
     * listener sockets. Make the listener sockets non-blocking, so that
     * child processes don't block in accept() when multiple processes are
     * selecting on the same socket and only one of them gets the connection.
     */
    switch (serv->type) {

	/*
	 * UNIX-domain or stream listener endpoints always come as singlets.
	 */
    case MASTER_SERV_TYPE_UNIX:
	set_eugid(var_owner_uid, var_owner_gid);
	serv->listen_fd[0] =
	    LOCAL_LISTEN(serv->name, serv->max_proc > var_proc_limit ?
			 serv->max_proc : var_proc_limit, NON_BLOCKING);
	close_on_exec(serv->listen_fd[0], CLOSE_ON_EXEC);
	set_ugid(getuid(), getgid());
	break;

	/*
	 * FIFO listener endpoints always come as singlets.
	 */
    case MASTER_SERV_TYPE_FIFO:
	set_eugid(var_owner_uid, var_owner_gid);
	serv->listen_fd[0] = fifo_listen(serv->name, 0622, NON_BLOCKING);
	close_on_exec(serv->listen_fd[0], CLOSE_ON_EXEC);
	set_ugid(getuid(), getgid());
	break;

	/*
	 * INET-domain listener endpoints can be wildcarded (the default) or
	 * bound to specific interface addresses.
	 * 
	 * With dual-stack IPv4/6 systems it does not matter, we have to specify
	 * the addresses anyway, either explicit or wild-card.
	 */
    case MASTER_SERV_TYPE_INET:
	for (n = 0; n < serv->listen_fd_count; n++) {
	    sa = SOCK_ADDR_PTR(MASTER_INET_ADDRLIST(serv)->addrs + n);
	    SOCKADDR_TO_HOSTADDR(sa, SOCK_ADDR_LEN(sa), &hostaddr,
				 (MAI_SERVPORT_STR *) 0, 0);
	    end_point = concatenate(hostaddr.buf,
				    ":", MASTER_INET_PORT(serv), (char *) 0);
	    serv->listen_fd[n]
		= inet_listen(end_point, serv->max_proc > var_proc_limit ?
			      serv->max_proc : var_proc_limit, NON_BLOCKING);
	    close_on_exec(serv->listen_fd[n], CLOSE_ON_EXEC);
	    myfree(end_point);
	}
	break;

	/*
	 * Descriptor passing endpoints always come as singlets.
	 */
#ifdef MASTER_SERV_TYPE_PASS
    case MASTER_SERV_TYPE_PASS:
	set_eugid(var_owner_uid, var_owner_gid);
	serv->listen_fd[0] =
	    PASS_LISTEN(serv->name, serv->max_proc > var_proc_limit ?
			serv->max_proc : var_proc_limit, NON_BLOCKING);
	close_on_exec(serv->listen_fd[0], CLOSE_ON_EXEC);
	set_ugid(getuid(), getgid());
	break;
#endif
    default:
	msg_panic("%s: unknown service type: %d", myname, serv->type);
    }
}

/* master_listen_cleanup - disable connection requests */

void    master_listen_cleanup(MASTER_SERV *serv)
{
    const char *myname = "master_listen_cleanup";
    int     n;

    /*
     * XXX The listen socket is shared with child processes. Closing the
     * socket in the master process does not really disable listeners in
     * child processes. There seems to be no documented way to turn off a
     * listener. The 4.4BSD shutdown(2) man page promises an ENOTCONN error
     * when shutdown(2) is applied to a socket that is not connected.
     */
    for (n = 0; n < serv->listen_fd_count; n++) {
	if (close(serv->listen_fd[n]) < 0)
	    msg_warn("%s: close listener socket %d: %m",
		     myname, serv->listen_fd[n]);
	serv->listen_fd[n] = -1;
    }
}
