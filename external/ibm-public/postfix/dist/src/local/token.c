/*	$NetBSD: token.c,v 1.1.1.1 2009/06/23 10:08:49 tron Exp $	*/

/*++
/* NAME
/*	token 3
/* SUMMARY
/*	tokenize alias/include/.forward entries and deliver
/* SYNOPSIS
/*	#include "local.h"
/*
/*	int	deliver_token(state, usr_attr, addr)
/*	LOCAL_STATE state;
/*	USER_ATTR usr_attr;
/*	TOK822	*addr;
/*
/*	int	deliver_token_string(state, usr_attr, string, addr_count)
/*	LOCAL_STATE state;
/*	USER_ATTR usr_attr;
/*	char	*string;
/*	int	*addr_count;
/*
/*	int	deliver_token_stream(state, usr_attr, fp, addr_count)
/*	LOCAL_STATE state;
/*	USER_ATTR usr_attr;
/*	VSTREAM	*fp;
/*	int	*addr_count;
/* DESCRIPTION
/*	This module delivers to addresses listed in an alias database
/*	entry, in an include file, or in a .forward file.
/*
/*	deliver_token() delivers to the address in the given token:
/*	an absolute /path/name, a ~/path/name relative to the recipient's
/*	home directory, an :include:/path/name request, an external
/*	"|command", or a mail address.
/*
/*	deliver_token_string() delivers to all addresses listed in
/*	the specified string.
/*
/*	deliver_token_stream() delivers to all addresses listed in
/*	the specified stream. Input records > \fIline_length_limit\fR
/*	are broken up into multiple records, to prevent the mail
/*	system from using unreasonable amounts of memory.
/*
/*	Arguments:
/* .IP state
/*	The attributes that specify the message, recipient and more.
/*	Attributes describing alias, include or forward expansion.
/*	A table with the results from expanding aliases or lists.
/*	A table with delivered-to: addresses taken from the message.
/* .IP usr_attr
/*	Attributes describing user rights and environment.
/* .IP addr
/*	A parsed address from an include file, alias file or .forward file.
/* .IP string
/*	A null-terminated string.
/* .IP fp
/*	A readable stream.
/* .IP addr_count
/*	Null pointer, or the address of a counter that is incremented
/*	by the number of destinations found by the tokenizer.
/* DIAGNOSTICS
/*	Fatal errors: out of memory. The result is non-zero when the
/*	operation should be tried again. Warnings: malformed address.
/* SEE ALSO
/*	list_token(3) tokenize list
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

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <htable.h>
#include <readlline.h>
#include <mymalloc.h>
#include <vstring_vstream.h>
#include <stringops.h>

/* Global library. */

#include <tok822.h>
#include <mail_params.h>
#include <bounce.h>
#include <defer.h>

/* Application-specific. */

#include "local.h"

/* deliver_token_home - expand ~token */

static int deliver_token_home(LOCAL_STATE state, USER_ATTR usr_attr, char *addr)
{
    char   *full_path;
    int     status;

    if (addr[1] != '/') {			/* disallow ~user */
	msg_warn("bad home directory syntax for: %s", addr);
	dsb_simple(state.msg_attr.why, "5.3.5",
		   "mail system configuration error");
	status = bounce_append(BOUNCE_FLAGS(state.request),
			       BOUNCE_ATTR(state.msg_attr));
    } else if (usr_attr.home == 0) {		/* require user context */
	msg_warn("unknown home directory for: %s", addr);
	dsb_simple(state.msg_attr.why, "5.3.5",
		   "mail system configuration error");
	status = bounce_append(BOUNCE_FLAGS(state.request),
			       BOUNCE_ATTR(state.msg_attr));
    } else if (usr_attr.home[0] == '/' && usr_attr.home[1] == 0) {
	status = deliver_file(state, usr_attr, addr + 1);
    } else {					/* expand ~ to home */
	full_path = concatenate(usr_attr.home, addr + 1, (char *) 0);
	status = deliver_file(state, usr_attr, full_path);
	myfree(full_path);
    }
    return (status);
}

/* deliver_token - deliver to expansion of include file or alias */

int     deliver_token(LOCAL_STATE state, USER_ATTR usr_attr, TOK822 *addr)
{
    VSTRING *addr_buf = vstring_alloc(100);
    static char include[] = ":include:";
    int     status;
    char   *path;

    tok822_internalize(addr_buf, addr->head, TOK822_STR_DEFL);
    if (msg_verbose)
	msg_info("deliver_token: %s", STR(addr_buf));

    if (*STR(addr_buf) == '/') {
	status = deliver_file(state, usr_attr, STR(addr_buf));
    } else if (*STR(addr_buf) == '~') {
	status = deliver_token_home(state, usr_attr, STR(addr_buf));
    } else if (*STR(addr_buf) == '|') {
	if ((local_cmd_deliver_mask & state.msg_attr.exp_type) == 0) {
	    dsb_simple(state.msg_attr.why, "5.7.1",
		       "mail to command is restricted");
	    status = bounce_append(BOUNCE_FLAGS(state.request),
				   BOUNCE_ATTR(state.msg_attr));
	} else
	    status = deliver_command(state, usr_attr, STR(addr_buf) + 1);
    } else if (strncasecmp(STR(addr_buf), include, sizeof(include) - 1) == 0) {
	path = STR(addr_buf) + sizeof(include) - 1;
	status = deliver_include(state, usr_attr, path);
    } else {
	status = deliver_resolve_tree(state, usr_attr, addr);
    }
    vstring_free(addr_buf);

    return (status);
}

/* deliver_token_string - tokenize string and deliver */

int     deliver_token_string(LOCAL_STATE state, USER_ATTR usr_attr,
			             char *string, int *addr_count)
{
    TOK822 *tree;
    TOK822 *addr;
    int     status = 0;

    if (msg_verbose)
	msg_info("deliver_token_string: %s", string);

    tree = tok822_parse(string);
    for (addr = tree; addr != 0; addr = addr->next) {
	if (addr->type == TOK822_ADDR) {
	    if (addr_count)
		(*addr_count)++;
	    status |= deliver_token(state, usr_attr, addr);
	}
    }
    tok822_free_tree(tree);
    return (status);
}

/* deliver_token_stream - tokenize stream and deliver */

int     deliver_token_stream(LOCAL_STATE state, USER_ATTR usr_attr,
			             VSTREAM *fp, int *addr_count)
{
    VSTRING *buf = vstring_alloc(100);
    int     status = 0;

    if (msg_verbose)
	msg_info("deliver_token_stream: %s", VSTREAM_PATH(fp));

    while (vstring_fgets_bound(buf, fp, var_line_limit)) {
	if (*STR(buf) != '#') {
	    status = deliver_token_string(state, usr_attr, STR(buf), addr_count);
	    if (status != 0)
		break;
	}
    }
    if (vstream_ferror(fp)) {
	dsb_simple(state.msg_attr.why, "4.3.0",
		   "error reading forwarding file: %m");
	status = defer_append(BOUNCE_FLAGS(state.request),
			      BOUNCE_ATTR(state.msg_attr));
    }
    vstring_free(buf);
    return (status);
}
