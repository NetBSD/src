/*++
/* NAME
/*	lmtp_session 3
/* SUMMARY
/*	LMTP_SESSION structure management
/* SYNOPSIS
/*	#include "lmtp.h"
/*
/*	LMTP_SESSION *lmtp_session_alloc(stream, host, addr, dest, type)
/*	VSTREAM *stream;
/*	const char *host;
/*	const char *addr;
/*	const char *dest;
/*	int	type;
/*
/*	LMTP_SESSION *lmtp_session_free(session)
/*	LMTP_SESSION *session;
/* DESCRIPTION
/*	This module maintains information about connections, including
/*	per-peer debugging.
/*
/*	lmtp_session_alloc() allocates memory for an LMTP_SESSION structure
/*	and initializes it with the given stream and host name and address
/*	information.  The host name and address strings are copied.
/*	The type argument specifies the transport type. The dest argument
/*	specifies a string-valued name for the remote endpoint.
/*	If the peer name or address matches the debug-peer_list configuration
/*	parameter, the debugging level is incremented by the amount specified
/*	in the debug_peer_level parameter.
/*
/*	lmtp_session_free() destroys an LMTP_SESSION structure and its
/*	members, making memory available for reuse. The result value is
/*	convenient null pointer. The debugging level is restored to the
/*	value prior to the lmtp_session_alloc() call.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* SEE ALSO
/*	debug_peer(3), increase logging for selected peers
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Alterations for LMTP by:
/*	Philip A. Prindeville
/*	Mirapoint, Inc.
/*	USA.
/*
/*	Additional work on LMTP by:
/*	Amos Gouaux
/*	University of Texas at Dallas
/*	P.O. Box 830688, MC34
/*	Richardson, TX 75083, USA
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <mymalloc.h>
#include <vstream.h>
#include <stringops.h>

/* Global library. */

#include <debug_peer.h>

/* Application-specific. */

#include "lmtp.h"

/* lmtp_session_alloc - allocate and initialize LMTP_SESSION structure */

LMTP_SESSION *lmtp_session_alloc(VSTREAM *stream, const char *host,
				         const char *addr, const char *dest)
{
    LMTP_SESSION *session;

    session = (LMTP_SESSION *) mymalloc(sizeof(*session));
    session->stream = stream;
    session->host = mystrdup(host);
    session->addr = mystrdup(addr);
    session->namaddr = concatenate(host, "[", addr, "]", (char *) 0);
    session->dest = mystrdup(dest);
    debug_peer_check(host, addr);
    return (session);
}

/* lmtp_session_free - destroy LMTP_SESSION structure and contents */

LMTP_SESSION *lmtp_session_free(LMTP_SESSION *session)
{
    debug_peer_restore();
    vstream_fclose(session->stream);
    myfree(session->host);
    myfree(session->addr);
    myfree(session->namaddr);
    myfree(session->dest);
    myfree((char *) session);
    return (0);
}
