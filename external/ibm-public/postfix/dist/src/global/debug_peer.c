/*	$NetBSD: debug_peer.c,v 1.1.1.1.16.1 2013/02/25 00:27:17 tls Exp $	*/

/*++
/* NAME
/*	debug_peer 3
/* SUMMARY
/*	increase verbose logging for specific peers
/* SYNOPSIS
/*	#include <debug_peer.h>
/*
/*	void	debug_peer_init(void)
/*
/*	int	peer_debug_check(peer_name, peer_addr)
/*	const char *peer_name;
/*	const char *peer_addr;
/*
/*	void	debug_peer_restore()
/* DESCRIPTION
/*	This module implements increased verbose logging for specific
/*	network peers.
/*
/*	The \fIdebug_peer_list\fR configuration parameter
/*	specifies what peers receive this special treatment; see
/*	namadr_list(3) for a description of the matching process.
/*
/*	The \fIdebug_peer_level\fR configuration parameter specifies
/*	by what amount the verbose logging level should increase when
/*	a peer is listed in \fIdebug_peer_list\fR.
/*
/*	debug_peer_init() performs initializations that must be
/*	performed once at the start of the program.
/*
/*	debug_peer_check() increases the verbose logging level when the
/*	client name or address matches the debug_peer_list pattern.
/*	The result is non-zero when the noise leven was increased.
/*
/*	debug_peer_restore() restores the verbose logging level.
/*	This routine has no effect when debug_peer_check() had no
/*	effect; this routine can safely be called multiple times.
/* DIAGNOSTICS
/*	Panic: interface violations.
/*	Fatal errors: unable to access a peer_list file; invalid
/*	peer_list pattern; invalid verbosity level increment.
/* SEE ALSO
/*	msg(3) the msg_verbose variable
/*	namadr_list(3) match host by name or by address
/* CONFIG PARAMETERS
/*	debug_peer_list, patterns as described in namadr_list(3)
/*	debug_peer_level, verbose logging level
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

#include <msg.h>

/* Global library. */

#include <mail_params.h>
#include <namadr_list.h>
#include <debug_peer.h>
#include <match_parent_style.h>

/* Application-specific. */

#define UNUSED_SAVED_LEVEL	(-1)

static NAMADR_LIST *debug_peer_list;
static int saved_level = UNUSED_SAVED_LEVEL;

/* debug_peer_init - initialize */

void    debug_peer_init(void)
{
    const char *myname = "debug_peer_init";

    /*
     * Sanity check.
     */
    if (debug_peer_list)
	msg_panic("%s: repeated call", myname);
    if (var_debug_peer_list == 0)
	msg_panic("%s: uninitialized %s", myname, VAR_DEBUG_PEER_LIST);
    if (var_debug_peer_level <= 0)
	msg_fatal("%s: %s <= 0", myname, VAR_DEBUG_PEER_LEVEL);

    /*
     * Finally.
     */
    if (*var_debug_peer_list)
	debug_peer_list =
	    namadr_list_init(MATCH_FLAG_RETURN
			     | match_parent_style(VAR_DEBUG_PEER_LIST),
			     var_debug_peer_list);
}

/* debug_peer_check - see if this peer needs verbose logging */

int     debug_peer_check(const char *name, const char *addr)
{

    /*
     * Crank up the noise when this peer is listed.
     */
    if (debug_peer_list != 0
	&& saved_level == UNUSED_SAVED_LEVEL
	&& namadr_list_match(debug_peer_list, name, addr) != 0) {
	saved_level = msg_verbose;
	msg_verbose += var_debug_peer_level;
	return (1);
    }
    return (0);
}

/* debug_peer_restore - restore logging level */

void    debug_peer_restore(void)
{
    if (saved_level != UNUSED_SAVED_LEVEL) {
	msg_verbose = saved_level;
	saved_level = UNUSED_SAVED_LEVEL;
    }
}
