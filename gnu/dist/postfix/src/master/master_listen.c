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
#include <iostuff.h>

/* Global library. */

#include <mail_params.h>

/* Application-specific. */

#include "master.h"

#ifdef INET6
#include <netdb.h>
#include <stdio.h>
#endif 

/* master_listen_init - enable connection requests */

void    master_listen_init(MASTER_SERV *serv)
{
    char   *myname = "master_listen_init";
    char   *end_point;
    int     n;
#ifdef INET6
    char hbuf[NI_MAXHOST];
    SOCKADDR_SIZE salen;
#endif

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
	set_eugid(getuid(), getgid());
	break;

	/*
	 * FIFO listener endpoints always come as singlets.
	 */
    case MASTER_SERV_TYPE_FIFO:
	set_eugid(var_owner_uid, var_owner_gid);
	serv->listen_fd[0] = fifo_listen(serv->name, 0622, NON_BLOCKING);
	close_on_exec(serv->listen_fd[0], CLOSE_ON_EXEC);
	set_eugid(getuid(), getgid());
	break;

	/*
	 * INET-domain listener endpoints can be wildcarded (the default) or
	 * bound to specific interface addresses.
	 */
    case MASTER_SERV_TYPE_INET:
	if (MASTER_INET_ADDRLIST(serv) == 0) {	/* wild-card */
	    serv->listen_fd[0] =
		inet_listen(MASTER_INET_PORT(serv),
			    serv->max_proc > var_proc_limit ?
			    serv->max_proc : var_proc_limit, NON_BLOCKING);
	    close_on_exec(serv->listen_fd[0], CLOSE_ON_EXEC);
	} else {				/* virtual or host:port */
	    for (n = 0; n < serv->listen_fd_count; n++) {
#ifdef INET6
#ifndef HAS_SA_LEN					
		salen = SA_LEN((struct sockaddr *)&MASTER_INET_ADDRLIST(serv)->addrs[n]);
#else			
		salen = ((struct sockaddr *)&MASTER_INET_ADDRLIST(serv)->addrs[n])->sa_len;
#endif			
		if (getnameinfo((struct sockaddr *)&MASTER_INET_ADDRLIST(serv)->addrs[n],
			salen, hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST)) {
		    strncpy(hbuf, "?????", sizeof(hbuf));
		}
		end_point = concatenate(hbuf, ":", serv->name, (char *) 0);
#else
		end_point = concatenate(inet_ntoa(MASTER_INET_ADDRLIST(serv)->addrs[n]),
					":", serv->name, (char *) 0);
#endif
		serv->listen_fd[n]
		    = inet_listen(end_point, serv->max_proc > var_proc_limit ?
			     serv->max_proc : var_proc_limit, NON_BLOCKING);
		close_on_exec(serv->listen_fd[n], CLOSE_ON_EXEC);
		myfree(end_point);
	    }
	}
	break;
    default:
	msg_panic("%s: unknown service type: %d", myname, serv->type);
    }
}

/* master_listen_cleanup - disable connection requests */

void    master_listen_cleanup(MASTER_SERV *serv)
{
    char   *myname = "master_listen_cleanup";
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
