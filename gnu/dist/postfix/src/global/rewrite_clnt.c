/*++
/* NAME
/*	rewrite_clnt 3
/* SUMMARY
/*	address rewrite service client
/* SYNOPSIS
/*	#include <vstring.h>
/*	#include <rewrite_clnt.h>
/*
/*	VSTRING	*rewrite_clnt(ruleset, address, result)
/*	const char *ruleset;
/*	const char *address;
/*
/*	VSTRING	*rewrite_clnt_internal(ruleset, address, result)
/*	const char *ruleset;
/*	const char *address;
/*	VSTRING	*result;
/* DESCRIPTION
/*	This module implements a mail address rewriting client.
/*
/*	rewrite_clnt() sends a rule set name and external-form address to the
/*	rewriting service and returns the resulting external-form address.
/*	In case of communication failure the program keeps trying until the
/*	mail system shuts down.
/*
/*	rewrite_clnt_internal() performs the same functionality but takes
/*	input in internal (unquoted) form, and produces output in internal
/*	(unquoted) form.
/* DIAGNOSTICS
/*	Warnings: communication failure. Fatal error: mail system is down.
/* SEE ALSO
/*	mail_proto(3h) low-level mail component glue.
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
#include <errno.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <events.h>
#include <iostuff.h>
#include <quote_822_local.h>

/* Global library. */

#include "mail_proto.h"
#include "mail_params.h"
#include "clnt_stream.h"
#include "rewrite_clnt.h"

/* Application-specific. */

 /*
  * XXX this is shared with the resolver client to save a file descriptor.
  */
CLNT_STREAM *rewrite_clnt_stream = 0;

static VSTRING *last_addr;
static VSTRING *last_result;

/* rewrite_clnt - rewrite address to (transport, next hop, recipient) */

VSTRING *rewrite_clnt(const char *rule, const char *addr, VSTRING *result)
{
    VSTREAM *stream;

    /*
     * One-entry cache.
     */
    if (last_addr == 0) {
	last_addr = vstring_alloc(100);
	last_result = vstring_alloc(100);
    }

    /*
     * Sanity check. An address must be in externalized form. The result must
     * not clobber the input, because we may have to retransmit the query.
     */
#define STR vstring_str

    if (*addr == 0)
	addr = "";
    if (addr == STR(result))
	msg_panic("rewrite_clnt: result clobbers input");

    /*
     * Peek at the cache.
     * 
     * XXX Must be made "rule" specific.
     */
    if (strcmp(addr, STR(last_addr)) == 0) {
	vstring_strcpy(result, STR(last_result));
	if (msg_verbose)
	    msg_info("rewrite_clnt: cached: %s: %s -> %s",
		     rule, addr, vstring_str(result));
	return (result);
    }

    /*
     * Keep trying until we get a complete response. The rewrite service is
     * CPU bound and making the client asynchronous would just complicate the
     * code.
     */
    if (rewrite_clnt_stream == 0)
	rewrite_clnt_stream = clnt_stream_create(MAIL_CLASS_PRIVATE,
						 var_rewrite_service,
						 var_ipc_idle_limit,
						 var_ipc_ttl_limit);

    for (;;) {
	stream = clnt_stream_access(rewrite_clnt_stream);
	errno = 0;
	if (attr_print(stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_STR, MAIL_ATTR_REQ, REWRITE_ADDR,
		       ATTR_TYPE_STR, MAIL_ATTR_RULE, rule,
		       ATTR_TYPE_STR, MAIL_ATTR_ADDR, addr,
		       ATTR_TYPE_END) != 0
	    || vstream_fflush(stream)
	    || attr_scan(stream, ATTR_FLAG_STRICT,
			 ATTR_TYPE_STR, MAIL_ATTR_ADDR, result,
			 ATTR_TYPE_END) != 1) {
	    if (msg_verbose || (errno != EPIPE && errno != ENOENT))
		msg_warn("problem talking to service %s: %m",
			 var_rewrite_service);
	} else {
	    if (msg_verbose)
		msg_info("rewrite_clnt: %s: %s -> %s",
			 rule, addr, vstring_str(result));
	    break;
	}
	sleep(1);				/* XXX make configurable */
	clnt_stream_recover(rewrite_clnt_stream);
    }

    /*
     * Update the cache.
     */
    vstring_strcpy(last_addr, addr);
    vstring_strcpy(last_result, STR(result));

    return (result);
}

/* rewrite_clnt_internal - rewrite from/to internal form */

VSTRING *rewrite_clnt_internal(const char *ruleset, const char *addr, VSTRING *result)
{
    VSTRING *src = vstring_alloc(100);
    VSTRING *dst = vstring_alloc(100);

    /*
     * Convert the address from internal address form to external RFC822
     * form, then rewrite it. After rewriting, convert to internal form.
     */
    quote_822_local(src, addr);
    rewrite_clnt(ruleset, STR(src), dst);
    unquote_822_local(result, STR(dst));
    vstring_free(src);
    vstring_free(dst);
    return (result);
}

#ifdef TEST

#include <stdlib.h>
#include <string.h>
#include <msg_vstream.h>
#include <split_at.h>
#include <vstring_vstream.h>
#include <mail_conf.h>
#include <mail_params.h>

static NORETURN usage(char *myname)
{
    msg_fatal("usage: %s [-v] [rule address...]", myname);
}

static void rewrite(char *rule, char *addr, VSTRING *reply)
{
    rewrite_clnt(rule, addr, reply);
    vstream_printf("%-10s %s\n", "rule", rule);
    vstream_printf("%-10s %s\n", "address", addr);
    vstream_printf("%-10s %s\n\n", "result", STR(reply));
    vstream_fflush(VSTREAM_OUT);
}

int     main(int argc, char **argv)
{
    VSTRING *reply;
    int     ch;
    char   *rule;
    char   *addr;

    msg_vstream_init(argv[0], VSTREAM_ERR);

    mail_conf_read();
    msg_info("using config files in %s", var_config_dir);
    if (chdir(var_queue_dir) < 0)
	msg_fatal("chdir %s: %m", var_queue_dir);

    while ((ch = GETOPT(argc, argv, "v")) > 0) {
	switch (ch) {
	case 'v':
	    msg_verbose++;
	    break;
	default:
	    usage(argv[0]);
	}
    }
    reply = vstring_alloc(1);

    if (argc > optind) {
	for (;;) {
	    if ((rule = argv[optind++]) == 0)
		break;
	    if ((addr = argv[optind++]) == 0)
		usage(argv[0]);
	    rewrite(rule, addr, reply);
	}
    } else {
	VSTRING *buffer = vstring_alloc(1);

	while (vstring_fgets_nonl(buffer, VSTREAM_IN)) {
	    if ((addr = split_at(STR(buffer), ' ')) == 0
		|| *(rule = STR(buffer)) == 0)
		usage(argv[0]);
	    rewrite(rule, addr, reply);
	}
	vstring_free(buffer);
    }
    vstring_free(reply);
    exit(0);
}

#endif
