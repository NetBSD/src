/*	$NetBSD: msg_stats_scan.c,v 1.1.1.1.2.2 2009/09/15 06:02:50 snj Exp $	*/

/*++
/* NAME
/*	msg_stats_scan
/* SUMMARY
/*	read MSG_STATS from stream
/* SYNOPSIS
/*	#include <msg_stats.h>
/*
/*	int	msg_stats_scan(scan_fn, stream, flags, ptr)
/*	ATTR_SCAN_MASTER_FN scan_fn;
/*	VSTREAM *stream;
/*	int	flags;
/*	void	*ptr;
/* DESCRIPTION
/*	msg_stats_scan() reads an MSG_STATS from the named stream
/*	using the specified attribute scan routine. msg_stats_scan()
/*	is meant to be passed as a call-back to attr_scan(), thusly:
/*
/*	... ATTR_SCAN_FUNC, msg_stats_scan, (void *) &stats, ...
/* DIAGNOSTICS
/*	Fatal: out of memory.
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

#include <attr.h>
#include <vstring.h>
#include <msg.h>

/* Global library. */

#include <mail_proto.h>
#include <msg_stats.h>

 /*
  * SLMs.
  */
#define STR(x) vstring_str(x)
#define LEN(x) VSTRING_LEN(x)

/* msg_stats_scan - read MSG_STATS from stream */

int     msg_stats_scan(ATTR_SCAN_MASTER_FN scan_fn, VSTREAM *fp,
		               int flags, void *ptr)
{
    MSG_STATS *stats = (MSG_STATS *) ptr;
    VSTRING *buf = vstring_alloc(sizeof(MSG_STATS) * 2);
    int     ret;

    /*
     * Receive the entire structure. This is not only simpler but also likely
     * to be quicker than having the sender figure out what fields need to be
     * sent, converting those numbers to string and back, and having the
     * receiver initialize the unused fields by hand.
     * 
     * XXX Would be nice if VSTRINGs could import a fixed-size buffer and
     * gracefully reject attempts to extend it.
     */
    ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
		  ATTR_TYPE_DATA, MAIL_ATTR_TIME, buf,
		  ATTR_TYPE_END);
    if (ret == 1) {
	if (LEN(buf) == sizeof(*stats)) {
	    memcpy((char *) stats, STR(buf), sizeof(*stats));
	} else {
	    msg_warn("msg_stats_scan: size mis-match: %u != %u",
		     (unsigned) LEN(buf), (unsigned) sizeof(*stats));
	    ret = (-1);
	}
    }
    vstring_free(buf);
    return (ret);
}
