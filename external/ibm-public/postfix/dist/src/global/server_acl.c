/*	$NetBSD: server_acl.c,v 1.1.1.1.2.2 2013/01/23 00:05:03 yamt Exp $	*/

/*++
/* NAME
/*	server_acl 3
/* SUMMARY
/*	server access list
/* SYNOPSIS
/*	#include <server_acl.h>
/*
/*	void	server_acl_pre_jail_init(mynetworks, param_name)
/*	const char *mynetworks;
/*	const char *param_name;
/*
/*	SERVER_ACL *server_acl_parse(extern_acl, param_name)
/*	const char *extern_acl;
/*	const char *param_name;
/*
/*	int	server_acl_eval(client_addr, intern_acl, param_name)
/*	const char *client_addr;
/*	SERVER_ACL *intern_acl;
/*	const char *param_name;
/* DESCRIPTION
/*	This module implements a permanent black/whitelist that
/*	is meant to be evaluated immediately after a client connects
/*	to a server.
/*
/*	server_acl_pre_jail_init() does before-chroot initialization
/*	for the permit_mynetworks setting.
/*
/*	server_acl_parse() converts an access list from raw string
/*	form to binary form. It should also be called as part of
/*	before-chroot initialization.
/*
/*	server_acl_eval() evaluates an access list for the specified
/*	client address.  The result is SERVER_ACL_ACT_PERMIT (permit),
/*	SERVER_ACL_ACT_REJECT (reject), SERVER_ACL_ACT_DUNNO (no
/*	decision), or SERVER_ACL_ACT_ERROR (error, unknown command
/*	or database access error).
/*
/*	Arguments:
/* .IP mynetworks
/*	Network addresses that match "permit_mynetworks".
/* .IP param_name
/*	The configuration parameter name for the access list from
/*	main.cf.  The information is used for error reporting (nested
/*	table, unknown keyword) and to select the appropriate
/*	behavior from parent_domain_matches_subdomains.
/* .IP extern_acl
/*	External access list representation.
/* .IP intern_acl
/*	Internal access list representation.
/* .IP client_addr
/*	The client IP address as printable string (without []).
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
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <dict.h>

/* Global library. */

#include <mail_params.h>
#include <addr_match_list.h>
#include <match_parent_style.h>
#include <server_acl.h>

/* Application-specific. */

#define SERVER_ACL_SEPARATORS	", \t\r\n"

static ADDR_MATCH_LIST *server_acl_mynetworks;

#define STR vstring_str

/* server_acl_pre_jail_init - initialize */

void    server_acl_pre_jail_init(const char *mynetworks, const char *origin)
{
    if (server_acl_mynetworks)
	addr_match_list_free(server_acl_mynetworks);
    server_acl_mynetworks =
	addr_match_list_init(MATCH_FLAG_RETURN | match_parent_style(origin),
			     mynetworks);
}

/* server_acl_parse - parse access list */

SERVER_ACL *server_acl_parse(const char *extern_acl, const char *origin)
{
    char   *saved_acl = mystrdup(extern_acl);
    SERVER_ACL *intern_acl = argv_alloc(1);
    char   *bp = saved_acl;
    char   *acl;

#define STREQ(x,y) (strcasecmp((x), (y)) == 0)
#define STRNE(x,y) (strcasecmp((x), (y)) != 0)

    /*
     * Nested tables are not allowed. Tables are opened before entering the
     * chroot jail, while access lists are evaluated after entering the
     * chroot jail.
     */
    while ((acl = mystrtok(&bp, SERVER_ACL_SEPARATORS)) != 0) {
	if (strchr(acl, ':') != 0) {
	    if (strchr(origin, ':') != 0) {
		msg_warn("table %s: lookup result \"%s\" is not allowed"
			 " -- ignoring remainder of access list",
			 origin, acl);
		argv_add(intern_acl, SERVER_ACL_NAME_DUNNO, (char *) 0);
		break;
	    } else {
		if (dict_handle(acl) == 0)
		    dict_register(acl, dict_open(acl, O_RDONLY, DICT_FLAG_LOCK
						 | DICT_FLAG_FOLD_FIX));
	    }
	}
	argv_add(intern_acl, acl, (char *) 0);
    }
    argv_terminate(intern_acl);

    /*
     * Cleanup.
     */
    myfree(saved_acl);
    return (intern_acl);
}

/* server_acl_eval - evaluate access list */

int     server_acl_eval(const char *client_addr, SERVER_ACL * intern_acl,
			        const char *origin)
{
    const char *myname = "server_acl_eval";
    char  **cpp;
    DICT   *dict;
    SERVER_ACL *argv;
    const char *acl;
    const char *dict_val;
    int     ret;

    for (cpp = intern_acl->argv; (acl = *cpp) != 0; cpp++) {
	if (msg_verbose)
	    msg_info("source=%s address=%s acl=%s",
		     origin, client_addr, acl);
	if (STREQ(acl, SERVER_ACL_NAME_REJECT)) {
	    return (SERVER_ACL_ACT_REJECT);
	} else if (STREQ(acl, SERVER_ACL_NAME_PERMIT)) {
	    return (SERVER_ACL_ACT_PERMIT);
	} else if (STREQ(acl, SERVER_ACL_NAME_WL_MYNETWORKS)) {
	    if (addr_match_list_match(server_acl_mynetworks, client_addr))
		return (SERVER_ACL_ACT_PERMIT);
	    if (server_acl_mynetworks->error != 0) {
		msg_warn("%s: %s: mynetworks lookup error -- ignoring the "
			 "remainder of this access list", origin, acl);
		return (SERVER_ACL_ACT_ERROR);
	    }
	} else if (strchr(acl, ':') != 0) {
	    if ((dict = dict_handle(acl)) == 0)
		msg_panic("%s: unexpected dictionary: %s", myname, acl);
	    if ((dict_val = dict_get(dict, client_addr)) != 0) {
		/* Fake up an ARGV to avoid lots of mallocs and frees. */
		if (dict_val[strcspn(dict_val, ":" SERVER_ACL_SEPARATORS)] == 0) {
		    ARGV_FAKE_BEGIN(fake_argv, dict_val);
		    ret = server_acl_eval(client_addr, &fake_argv, acl);
		    ARGV_FAKE_END;
		} else {
		    argv = server_acl_parse(dict_val, acl);
		    ret = server_acl_eval(client_addr, argv, acl);
		    argv_free(argv);
		}
		if (ret != SERVER_ACL_ACT_DUNNO)
		    return (ret);
	    } else if (dict->error != 0) {
		msg_warn("%s: %s: table lookup error -- ignoring the remainder "
			 "of this access list", origin, acl);
		return (SERVER_ACL_ACT_ERROR);
	    }
	} else if (STREQ(acl, SERVER_ACL_NAME_DUNNO)) {
	    return (SERVER_ACL_ACT_DUNNO);
	} else {
	    msg_warn("%s: unknown command: %s -- ignoring the remainder "
		     "of this access list", origin, acl);
	    return (SERVER_ACL_ACT_ERROR);
	}
    }
    if (msg_verbose)
	msg_info("source=%s address=%s - no match",
		 origin, client_addr);
    return (SERVER_ACL_ACT_DUNNO);
}

 /*
  * Access lists need testing. Not only with good inputs; error cases must
  * also be handled appropriately.
  */
#ifdef TEST
#include <unistd.h>
#include <stdlib.h>
#include <vstring_vstream.h>
#include <name_code.h>
#include <split_at.h>

char   *var_par_dom_match = DEF_PAR_DOM_MATCH;
char   *var_mynetworks = "";
char   *var_server_acl = "";

#define UPDATE_VAR(s,v) do { if (*(s)) myfree(s); (s) = mystrdup(v); } while (0)

int     main(void)
{
    VSTRING *buf = vstring_alloc(100);
    SERVER_ACL *argv;
    int     ret;
    int     have_tty = isatty(0);
    char   *bufp;
    char   *cmd;
    char   *value;
    const NAME_CODE acl_map[] = {
	SERVER_ACL_NAME_ERROR, SERVER_ACL_ACT_ERROR,
	SERVER_ACL_NAME_PERMIT, SERVER_ACL_ACT_PERMIT,
	SERVER_ACL_NAME_REJECT, SERVER_ACL_ACT_REJECT,
	SERVER_ACL_NAME_DUNNO, SERVER_ACL_ACT_DUNNO,
	0,
    };

#define VAR_SERVER_ACL "server_acl"

    while (vstring_get_nonl(buf, VSTREAM_IN) != VSTREAM_EOF) {
	bufp = STR(buf);
	if (have_tty == 0) {
	    vstream_printf("> %s\n", bufp);
	    vstream_fflush(VSTREAM_OUT);
	}
	if (*bufp == '#')
	    continue;
	if ((cmd = mystrtok(&bufp, " =")) == 0 || STREQ(cmd, "?")) {
	    vstream_printf("usage: %s=value|%s=value|address=value\n",
			   VAR_MYNETWORKS, VAR_SERVER_ACL);
	} else if ((value = mystrtok(&bufp, " =")) == 0) {
	    vstream_printf("missing value\n");
	} else if (STREQ(cmd, VAR_MYNETWORKS)) {
	    UPDATE_VAR(var_mynetworks, value);
	} else if (STREQ(cmd, VAR_SERVER_ACL)) {
	    UPDATE_VAR(var_server_acl, value);
	} else if (STREQ(cmd, "address")) {
	    server_acl_pre_jail_init(var_mynetworks, VAR_SERVER_ACL);
	    argv = server_acl_parse(var_server_acl, VAR_SERVER_ACL);
	    ret = server_acl_eval(value, argv, VAR_SERVER_ACL);
	    argv_free(argv);
	    vstream_printf("%s: %s\n", value, str_name_code(acl_map, ret));
	} else {
	    vstream_printf("unknown command: \"%s\"\n", cmd);
	}
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(buf);
    exit(0);
}

#endif
