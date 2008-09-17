/*++
/* NAME
/*	delivered_hdr 3
/* SUMMARY
/*	process Delivered-To: headers
/* SYNOPSIS
/*	#include <delivered_hdr.h>
/*
/*	DELIVERED_HDR_INFO *delivered_hdr_init(stream, offset, flags)
/*	VSTREAM	*stream;
/*	off_t	offset;
/*	int	flags;
/*
/*	int	delivered_hdr_find(info, address)
/*	DELIVERED_HDR_INFO *info;
/*	const char *address;
/*
/*	void	delivered_hdr_free(info)
/*	DELIVERED_HDR_INFO *info;
/* DESCRIPTION
/*	This module processes addresses in Delivered-To: headers.
/*	These headers are added by some mail delivery systems, for the
/*	purpose of breaking mail forwarding loops. N.B. This solves
/*	a different problem than the Received: hop count limit. Hop
/*	counts are used to limit the impact of mail routing problems.
/*
/*	delivered_hdr_init() extracts Delivered-To: header addresses
/*	from the specified message, and returns a table with the
/*	result. The file seek pointer is changed.
/*
/*	delivered_hdr_find() looks up the address in the lookup table,
/*	and returns non-zero when the address was found. The
/*	address argument must be in internalized form.
/*
/*	delivered_hdr_free() releases storage that was allocated by
/*	delivered_hdr_init().
/*
/*	Arguments:
/* .IP stream
/*	The open queue file.
/* .IP offset
/*	Offset of the first message content record.
/* .IP flags
/*	Zero, or the bit-wise OR ot:
/* .RS
/* .IP FOLD_ADDR_USER
/*	Case fold the address local part.
/* .IP FOLD_ADDR_HOST
/*	Case fold the address domain part.
/* .IP FOLD_ADDR_ALL
/*	Alias for (FOLD_ADDR_USER | FOLD_ADDR_HOST).
/* .RE
/* .IP info
/*	Extracted Delivered-To: addresses information.
/* .IP address
/*	A recipient address, internal form.
/* DIAGNOSTICS
/*	Fatal errors: out of memory.
/* SEE ALSO
/*	mail_copy(3), producer of Delivered-To: and other headers.
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
#include <mymalloc.h>
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
#include <delivered_hdr.h>
#include <fold_addr.h>

 /*
  * Application-specific.
  */
struct DELIVERED_HDR_INFO {
    int     flags;
    VSTRING *buf;
    HTABLE *table;
};

#define STR(x) vstring_str(x)

/* delivered_hdr_init - extract delivered-to information from the message */

DELIVERED_HDR_INFO *delivered_hdr_init(VSTREAM *fp, off_t offset, int flags)
{
    char   *cp;
    DELIVERED_HDR_INFO *info;
    const HEADER_OPTS *hdr;

    /*
     * Sanity check.
     */
    info = (DELIVERED_HDR_INFO *) mymalloc(sizeof(*info));
    info->flags = flags;
    info->buf = vstring_alloc(10);
    info->table = htable_create(0);

    if (vstream_fseek(fp, offset, SEEK_SET) < 0)
	msg_fatal("seek queue file %s: %m", VSTREAM_PATH(fp));

    /*
     * XXX Assume that mail_copy() produces delivered-to headers that fit in
     * a REC_TYPE_NORM record. Lowercase the delivered-to addresses for
     * consistency.
     * 
     * XXX Don't get bogged down by gazillions of delivered-to headers.
     */
#define DELIVERED_HDR_LIMIT	1000

    while (rec_get(fp, info->buf, 0) == REC_TYPE_NORM
	   && info->table->used < DELIVERED_HDR_LIMIT) {
	if (is_header(STR(info->buf))) {
	    if ((hdr = header_opts_find(STR(info->buf))) != 0
		&& hdr->type == HDR_DELIVERED_TO) {
		cp = STR(info->buf) + strlen(hdr->name) + 1;
		while (ISSPACE(*cp))
		    cp++;
		if (info->flags & FOLD_ADDR_ALL)
		    fold_addr(cp, info->flags);
		if (msg_verbose)
		    msg_info("delivered_hdr_init: %s", cp);
		htable_enter(info->table, cp, (char *) 0);
	    }
	} else if (ISSPACE(STR(info->buf)[0])) {
	    continue;
	} else {
	    break;
	}
    }
    return (info);
}

/* delivered_hdr_find - look up recipient in delivered table */

int     delivered_hdr_find(DELIVERED_HDR_INFO *info, const char *address)
{
    HTABLE_INFO *ht;

    /*
     * mail_copy() uses quote_822_local() when writing the Delivered-To:
     * header. We must therefore apply the same transformation when looking
     * up the recipient. Lowercase the delivered-to address for consistency.
     */
    quote_822_local(info->buf, address);
    if (info->flags & FOLD_ADDR_ALL)
	fold_addr(STR(info->buf), info->flags);
    ht = htable_locate(info->table, STR(info->buf));
    return (ht != 0);
}

/* delivered_hdr_free - destructor */

void    delivered_hdr_free(DELIVERED_HDR_INFO *info)
{
    vstring_free(info->buf);
    htable_free(info->table, (void (*) (char *)) 0);
    myfree((char *) info);
}
