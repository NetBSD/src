/*++
/* NAME
/*	qmqpd_peer 3
/* SUMMARY
/*	look up peer name/address information
/* SYNOPSIS
/*	#include "qmqpd.h"
/*
/*	void	qmqpd_peer_init(state)
/*	QMQPD_STATE *state;
/*
/*	void	qmqpd_peer_reset(state)
/*	QMQPD_STATE *state;
/* DESCRIPTION
/*	The qmqpd_peer_init() routine attempts to produce a printable
/*	version of the peer name and address of the specified socket.
/*	Where information is unavailable, the name and/or address
/*	are set to "unknown".
/*
/*	qmqpd_peer_init() updates the following fields:
/* .IP name
/*	The client hostname. An unknown name is represented by the
/*	string "unknown".
/* .IP addr
/*	Printable representation of the client address.
/* .IP namaddr
/*	String of the form: "name[addr]".
/* .PP
/*	qmqpd_peer_reset() releases memory allocate by qmqpd_peer_init().
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
#include <stdio.h>			/* strerror() */
#include <errno.h>
#include <netdb.h>
#include <string.h>

 /*
  * Older systems don't have h_errno. Even modern systems don't have
  * hstrerror().
  */
#ifdef NO_HERRNO

static int h_errno = TRY_AGAIN;

#define  HSTRERROR(err) "Host not found"

#else

#define  HSTRERROR(err) (\
	err == TRY_AGAIN ? "Host not found, try again" : \
	err == HOST_NOT_FOUND ? "Host not found" : \
	err == NO_DATA ? "Host name has no address" : \
	err == NO_RECOVERY ? "Name server failure" : \
	strerror(errno) \
    )
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <valid_hostname.h>
#include <stringops.h>

/* Global library. */


/* Application-specific. */

#include "qmqpd.h"

/* qmqpd_peer_init - initialize peer information */

void    qmqpd_peer_init(QMQPD_STATE *state)
{
    struct sockaddr_in sin;
    SOCKADDR_SIZE len = sizeof(sin);
    struct hostent *hp;
    int     i;

    /*
     * Look up the peer address information.
     */
    if (getpeername(vstream_fileno(state->client),
		    (struct sockaddr *) & sin, &len) >= 0) {
	errno = 0;
    }

    /*
     * If peer went away, give up.
     */
    if (errno == ECONNRESET || errno == ECONNABORTED) {
	state->name = mystrdup("unknown");
	state->addr = mystrdup("unknown");
    }

    /*
     * Look up and "verify" the client hostname.
     */
    else if (errno == 0 && sin.sin_family == AF_INET) {
	state->addr = mystrdup(inet_ntoa(sin.sin_addr));
	hp = gethostbyaddr((char *) &(sin.sin_addr),
			   sizeof(sin.sin_addr), AF_INET);
	if (hp == 0) {
	    state->name = mystrdup("unknown");
	} else if (!valid_hostname(hp->h_name, DONT_GRIPE)) {
	    state->name = mystrdup("unknown");
	} else {
	    state->name = mystrdup(hp->h_name);	/* hp->name is clobbered!! */

	    /*
	     * Reject the hostname if it does not list the peer address.
	     */
#define REJECT_PEER_NAME(state) { \
	myfree(state->name); \
	state->name = mystrdup("unknown"); \
    }

	    hp = gethostbyname(state->name);	/* clobbers hp->name!! */
	    if (hp == 0) {
		msg_warn("%s: hostname %s verification failed: %s",
			 state->addr, state->name, HSTRERROR(h_errno));
		REJECT_PEER_NAME(state);
	    } else if (hp->h_length != sizeof(sin.sin_addr)) {
		msg_warn("%s: hostname %s verification failed: bad address size %d",
			 state->addr, state->name, hp->h_length);
		REJECT_PEER_NAME(state);
	    } else {
		for (i = 0; /* void */ ; i++) {
		    if (hp->h_addr_list[i] == 0) {
			msg_warn("%s: address not listed for hostname %s",
				 state->addr, state->name);
			REJECT_PEER_NAME(state);
			break;
		    }
		    if (memcmp(hp->h_addr_list[i],
			       (char *) &sin.sin_addr,
			       sizeof(sin.sin_addr)) == 0)
			break;			/* keep peer name */
		}
	    }
	}
    }

    /*
     * If it's not Internet, assume the client is local, and avoid using the
     * naming service because that can hang when the machine is disconnected.
     */
    else {
	state->name = mystrdup("localhost");
	state->addr = mystrdup("127.0.0.1");	/* XXX bogus. */
    }

    /*
     * Do the name[addr] formatting for pretty reports.
     */
    state->namaddr =
	concatenate(state->name, "[", state->addr, "]", (char *) 0);
}

/* qmqpd_peer_reset - destroy peer information */

void    qmqpd_peer_reset(QMQPD_STATE *state)
{
    myfree(state->name);
    myfree(state->addr);
    myfree(state->namaddr);
}
