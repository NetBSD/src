/*++
/* NAME
/*	smtp_session 3
/* SUMMARY
/*	SMTP_SESSION structure management
/* SYNOPSIS
/*	#include "smtp.h"
/*
/*	SMTP_SESSION *smtp_session_alloc(stream, host, addr)
/*	VSTREAM *stream;
/*	char	*host;
/*	char	*addr;
/*
/*	void	smtp_session_free(session)
/*	SMTP_SESSION *session;
/* DESCRIPTION
/*	smtp_session_alloc() allocates memory for an SMTP_SESSION structure
/*	and initializes it with the given stream and host name and address
/*	information.  The host name and address strings are copied. The code
/*	assumes that the stream is connected to the "best" alternative.
/*
/*	smtp_session_free() destroys an SMTP_SESSION structure and its
/*	members, making memory available for reuse.
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

/* Utility library. */

#include <mymalloc.h>
#include <vstream.h>
#include <stringops.h>

/* Application-specific. */

#include "smtp.h"

/* smtp_session_alloc - allocate and initialize SMTP_SESSION structure */

SMTP_SESSION *smtp_session_alloc(VSTREAM *stream, char *host, char *addr)
{
    SMTP_SESSION *session;

    session = (SMTP_SESSION *) mymalloc(sizeof(*session));
    session->stream = stream;
    session->host = mystrdup(host);
    session->addr = mystrdup(addr);
    session->namaddr = concatenate(host, "[", addr, "]", (char *) 0);
    session->best = 1;
    return (session);
}

/* smtp_session_free - destroy SMTP_SESSION structure and contents */

void    smtp_session_free(SMTP_SESSION *session)
{
    vstream_fclose(session->stream);
    myfree(session->host);
    myfree(session->addr);
    myfree(session->namaddr);
    myfree((char *) session);
}
