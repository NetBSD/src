/*++
/* NAME
/*	smtpd_peer 3
/* SUMMARY
/*	look up peer name/address information
/* SYNOPSIS
/*	#include "smtpd.h"
/*
/*	void	smtpd_peer_init(state)
/*	SMTPD_STATE *state;
/*
/*	void	smtpd_peer_reset(state)
/*	SMTPD_STATE *state;
/* DESCRIPTION
/*	The smtpd_peer_init() routine attempts to produce a printable
/*	version of the peer name and address of the specified socket.
/*	Where information is unavailable, the name and/or address
/*	are set to "unknown".
/*
/*	smtpd_peer_init() updates the following fields:
/* .IP name
/*	The client hostname. An unknown name is represented by the
/*	string "unknown".
/* .IP addr
/*	Printable representation of the client address.
/* .IP namaddr
/*	String of the form: "name[addr]".
/* .IP peer_code
/*	The peer_code result field specifies how the client name
/*	information should be interpreted:
/* .RS
/* .IP 2
/*	Both name lookup and name verification succeeded.
/* .IP 4
/*	The name lookup or name verification failed with a recoverable
/*	error (no address->name mapping or no name->address mapping).
/* .IP 5
/*	The name lookup or verification failed with an unrecoverable
/*	error (no address->name mapping, bad hostname syntax, no
/*	name->address mapping, client address not listed for hostname).
/* .RE
/* .PP
/*	smtpd_peer_reset() releases memory allocate by smtpd_peer_init().
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

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <valid_hostname.h>
#include <stringops.h>

/* Global library. */

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


/* Application-specific. */

#include "smtpd.h"

/* smtpd_peer_init - initialize peer information */

void    smtpd_peer_init(SMTPD_STATE *state)
{
#ifdef INET6
    struct sockaddr_storage ss;
#else
    struct sockaddr ss;
    struct in_addr *in;
    struct hostent *hp;
#endif
    struct sockaddr *sa;
    SOCKADDR_SIZE len;

    sa = (struct sockaddr *)&ss;
    len = sizeof(ss);

    /*
     * Look up the peer address information.
     */
    if (getpeername(vstream_fileno(state->client), sa, &len) >= 0) {
	errno = 0;
    }

    /*
     * If peer went away, give up.
     */
    if (errno == ECONNRESET || errno == ECONNABORTED) {
	msg_info("errno %d %m", errno);
	state->name = mystrdup("unknown");
	state->addr = mystrdup("unknown");
	state->peer_code = 5;
    }

    /*
     * Look up and "verify" the client hostname.
     */
    else if (errno == 0 && (sa->sa_family == AF_INET
#ifdef INET6
			    || sa->sa_family == AF_INET6
#endif
		    )) {
#ifdef INET6
	char hbuf[NI_MAXHOST];
	struct addrinfo hints, *rnull = NULL;
#else
	char hbuf[sizeof("255.255.255.255") + 1];
#endif
	int error = -1;

#ifdef INET6
	(void)getnameinfo(sa, len, hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST);
#else
	in = &((struct sockaddr_in *)sa)->sin_addr;
	inet_ntop(AF_INET, in, hbuf, sizeof(hbuf));
#endif
	state->addr = mystrdup(hbuf);
#ifdef INET6
	error = getnameinfo(sa, len, hbuf, sizeof(hbuf), NULL, 0, NI_NAMEREQD);
#else
	hp = gethostbyaddr((char *)in, sizeof(*in), AF_INET);
	if (hp && strlen(hp->h_name) < sizeof(hbuf) - 1) {
	    error = 0;
	    strncpy(hbuf, hp->h_name, sizeof(hbuf) - 1);
	    hbuf[sizeof(hbuf) - 1] = '\0';
	} else
	    error = 1;
#endif
	if (error) {
	    state->name = mystrdup("unknown");
	    state->peer_code = (h_errno == TRY_AGAIN ? 4 : 5);
	} else if (!valid_hostname(hbuf, DONT_GRIPE)) {
	    state->name = mystrdup("unknown");
	    state->peer_code = 5;
	} else {
	    state->name = mystrdup(hbuf);	/* hp->name is clobbered!! */
	    state->peer_code = 2;

	    /*
	     * Reject the hostname if it does not list the peer address.
	     */
#define REJECT_PEER_NAME(state, code) { \
	myfree(state->name); \
	state->name = mystrdup("unknown"); \
	state->peer_code = code; \
    }

#ifdef INET6
	    memset(&hints, 0, sizeof(hints));
	    hints.ai_family = AF_UNSPEC;
	    hints.ai_socktype = SOCK_STREAM;
	    error = getaddrinfo(state->name, NULL, &hints, &rnull);
	    if (error) {
		msg_warn("%s: hostname %s verification failed: %s",
			 state->addr, state->name, gai_strerror(error));
		REJECT_PEER_NAME(state, (error == EAI_AGAIN ? 4 : 5));
	    }
	    /* memcmp() isn't needed if we use getaddrinfo */
	    if (rnull)
		freeaddrinfo(rnull);
#else
	    hp = gethostbyname(state->name);	/* clobbers hp->name!! */
	    if (hp == 0) {
		msg_warn("%s: hostname %s verification failed: %s",
			 state->addr, state->name, HSTRERROR(h_errno));
		REJECT_PEER_NAME(state, (h_errno == TRY_AGAIN ? 4 : 5));
	    } else if (hp->h_length != sizeof(*in)) {
		msg_warn("%s: hostname %s verification failed: bad address size %d",
			 state->addr, state->name, hp->h_length);
		REJECT_PEER_NAME(state, 5);
	    } else {
		int i;
		for (i = 0; /* void */ ; i++) {
		    if (hp->h_addr_list[i] == 0) {
			msg_warn("%s: address not listed for hostname %s",
				 state->addr, state->name);
			REJECT_PEER_NAME(state, 5);
			break;
		    }
		    if (memcmp(hp->h_addr_list[i], (char *)in, sizeof(*in)) == 0)
			break;			/* keep peer name */
		}
	    }
#endif
	}
    }

    /*
     * If it's not Internet, assume the client is local, and avoid using the
     * naming service because that can hang when the machine is disconnected.
     */
    else {
	state->name = mystrdup("localhost");
	state->addr = mystrdup("127.0.0.1");	/* XXX bogus. */
	state->peer_code = 2;
    }

    /*
     * Do the name[addr] formatting for pretty reports.
     */
    state->namaddr =
	concatenate(state->name, "[", state->addr, "]", (char *) 0);
}

/* smtpd_peer_reset - destroy peer information */

void    smtpd_peer_reset(SMTPD_STATE *state)
{
    myfree(state->name);
    myfree(state->addr);
    myfree(state->namaddr);
}
