/*++
/* NAME
/*	delivered 3
/* SUMMARY
/*	process Delivered-To: headers
/* SYNOPSIS
/*	#include "local.h"
/*
/*	HTABLE	*delivered_init(attr)
/*	DELIVER_ATTR attr;
/*
/*	int	delivered_find(table, address)
/*	HTABLE	*table;
/*	char	*address;
/*
/*	void	delivered_free(table)
/*	HTABLE	*table;
/* DESCRIPTION
/*	This module processes addresses in Delivered-To: headers.
/*	These headers are added by some mail delivery systems, for the
/*	purpose of breaking mail forwarding loops. N.B. This solves
/*	a different problem than the Received: hop count limit. Hop
/*	counts are used to limit the impact of mail routing problems.
/*
/*	delivered_init() extracts Delivered-To: header addresses
/*	from the specified message, and returns a table with the
/*	result.
/*
/*	delivered_find() looks up the address in the lookup table,
/*	and returns non-zero when the address was found. The
/*	address argument must be in internalized form.
/*
/*	delivered_free() releases storage that was allocated by
/*	delivered_init().
/*
/*	Arguments:
/* .IP state
/*	The attributes that specify the message, recipient and more.
/* .IP table
/*	A table with extracted Delivered-To: addresses.
/* .IP address
/*	A recipient address, internal form.
/* DIAGNOSTICS
/*	Fatal errors: out of memory.
/* SEE ALSO
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
#include <unistd.h>
#include <string.h>
#include <ctype.h>

/* Utility library. */

#include <msg.h>
#include <htable.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <stringops.h>

/* Global library. */

#include <record.h>
#include <rec_type.h>
#include <is_header.h>
#include <quote_822_local.h>
#include <header_opts.h>

/* Application-specific. */

#include "local.h"

static VSTRING *buf;

#define STR vstring_str

/* delivered_init - extract delivered-to information from the message */

HTABLE *delivered_init(DELIVER_ATTR attr)
{
    char   *cp;
    HTABLE *table = htable_create(0);
    HEADER_OPTS *hdr;

    if (buf == 0)
	buf = vstring_alloc(10);

    if (vstream_fseek(attr.fp, attr.offset, SEEK_SET) < 0)
	msg_fatal("seek queue file %s: %m", VSTREAM_PATH(attr.fp));

    /*
     * XXX Assume that normal mail systems produce headers that fit in a
     * REC_TYPE_NORM record. Lowercase the delivered-to addresses for
     * consistency.
     */
    while (rec_get(attr.fp, buf, 0) == REC_TYPE_NORM) {
	if (is_header(STR(buf))) {
	    if ((hdr = header_opts_find(STR(buf))) != 0
		&& hdr->type == HDR_DELIVERED_TO) {
		cp = STR(buf) + strlen(hdr->name) + 1;
		while (ISSPACE(*cp))
		    cp++;
		lowercase(cp);
		if (msg_verbose)
		    msg_info("delivered_init: %s", cp);
		htable_enter(table, cp, (char *) 0);
	    }
	} else if (ISSPACE(STR(buf)[0])) {
	    continue;
	} else {
	    break;
	}
    }
    return (table);
}

/* delivered_find - look up recipient in delivered table */

int     delivered_find(HTABLE *table, char *address)
{
    HTABLE_INFO *ht;

    /*
     * mail_copy() uses quote_822_local() when writing the Delivered-To:
     * header. We must therefore apply the same transformation when looking
     * up the recipient. Lowercase the delivered-to address for consistency.
     */
    quote_822_local(buf, address);
    lowercase(STR(buf));
    ht = htable_locate(table, STR(buf));
    return (ht != 0);
}
