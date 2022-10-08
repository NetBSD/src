/*	$NetBSD: delivered_hdr.c,v 1.3 2022/10/08 16:12:45 christos Exp $	*/

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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
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
    VSTRING *fold;
    HTABLE *table;
};

#define STR(x) vstring_str(x)

/* delivered_hdr_init - extract delivered-to information from the message */

DELIVERED_HDR_INFO *delivered_hdr_init(VSTREAM *fp, off_t offset, int flags)
{
    char   *cp;
    DELIVERED_HDR_INFO *info;
    const HEADER_OPTS *hdr;
    int     curr_type;
    int     prev_type;

    /*
     * Sanity check.
     */
    info = (DELIVERED_HDR_INFO *) mymalloc(sizeof(*info));
    info->flags = flags;
    info->buf = vstring_alloc(10);
    info->fold = vstring_alloc(10);
    info->table = htable_create(0);

    if (vstream_fseek(fp, offset, SEEK_SET) < 0)
	msg_fatal("seek queue file %s: %m", VSTREAM_PATH(fp));

    /*
     * XXX Assume that mail_copy() produces delivered-to headers that fit in
     * a REC_TYPE_NORM or REC_TYPE_CONT record. Lowercase the delivered-to
     * addresses for consistency.
     * 
     * XXX Don't get bogged down by gazillions of delivered-to headers.
     */
#define DELIVERED_HDR_LIMIT	1000

    for (prev_type = REC_TYPE_NORM;
	 info->table->used < DELIVERED_HDR_LIMIT
	 && ((curr_type = rec_get(fp, info->buf, 0)) == REC_TYPE_NORM
	     || curr_type == REC_TYPE_CONT);
	 prev_type = curr_type) {
	if (prev_type == REC_TYPE_CONT)
	    continue;
	if (is_header(STR(info->buf))) {
	    if ((hdr = header_opts_find(STR(info->buf))) != 0
		&& hdr->type == HDR_DELIVERED_TO) {
		cp = STR(info->buf) + strlen(hdr->name) + 1;
		while (ISSPACE(*cp))
		    cp++;
		cp = fold_addr(info->fold, cp, info->flags);
		if (msg_verbose)
		    msg_info("delivered_hdr_init: %s", cp);
		htable_enter(info->table, cp, (void *) 0);
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
    const char *addr_key;

    /*
     * mail_copy() uses quote_822_local() when writing the Delivered-To:
     * header. We must therefore apply the same transformation when looking
     * up the recipient. Lowercase the delivered-to address for consistency.
     */
    quote_822_local(info->buf, address);
    addr_key = fold_addr(info->fold, STR(info->buf), info->flags);
    ht = htable_locate(info->table, addr_key);
    return (ht != 0);
}

/* delivered_hdr_free - destructor */

void    delivered_hdr_free(DELIVERED_HDR_INFO *info)
{
    vstring_free(info->buf);
    vstring_free(info->fold);
    htable_free(info->table, (void (*) (void *)) 0);
    myfree((void *) info);
}

#ifdef TEST

#include <msg_vstream.h>
#include <mail_params.h>

char   *var_drop_hdrs;

int     main(int arc, char **argv)
{

    /*
     * We write test records to a VSTRING, then read with delivered_hdr_init.
     */
    VSTRING *mem_bp;
    VSTREAM *mem_fp;
    DELIVERED_HDR_INFO *dp;
    struct test_case {
	int     rec_type;
	const char *content;
	int     expect_find;
    };
    const struct test_case test_cases[] = {
	REC_TYPE_CONT, "Delivered-To: one", 1,
	REC_TYPE_NORM, "Delivered-To: two", 0,
	REC_TYPE_NORM, "Delivered-To: three", 1,
	0,
    };
    const struct test_case *tp;
    int     actual_find;
    int     errors;

    msg_vstream_init(argv[0], VSTREAM_ERR);

    var_drop_hdrs = mystrdup(DEF_DROP_HDRS);

    mem_bp = vstring_alloc(VSTREAM_BUFSIZE);
    if ((mem_fp = vstream_memopen(mem_bp, O_WRONLY)) == 0)
	msg_panic("vstream_memopen O_WRONLY failed: %m");

#define REC_PUT_LIT(fp, type, lit) rec_put((fp), (type), (lit), strlen(lit))

    for (tp = test_cases; tp->content != 0; tp++)
	REC_PUT_LIT(mem_fp, tp->rec_type, tp->content);

    if (vstream_fclose(mem_fp))
	msg_panic("vstream_fclose fail: %m");

    if ((mem_fp = vstream_memopen(mem_bp, O_RDONLY)) == 0)
	msg_panic("vstream_memopen O_RDONLY failed: %m");

    dp = delivered_hdr_init(mem_fp, 0, FOLD_ADDR_ALL);

    for (errors = 0, tp = test_cases; tp->content != 0; tp++) {
	actual_find =
	    delivered_hdr_find(dp, tp->content + sizeof("Delivered-To:"));
	msg_info("test case: %c >%s<: %s (expected: %s)",
		 tp->rec_type, tp->content,
		 actual_find == tp->expect_find ? "PASS" : "FAIL",
		 tp->expect_find ? "MATCH" : "NO MATCH");
	errors += (actual_find != tp->expect_find);;
    }
    exit(errors);
}

#endif
