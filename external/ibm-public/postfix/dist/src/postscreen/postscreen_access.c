/*	$NetBSD: postscreen_access.c,v 1.1.1.2 2012/12/18 09:02:14 tron Exp $	*/

/*++
/* NAME
/*	postscreen_access 3
/* SUMMARY
/*	postscreen access list support
/* SYNOPSIS
/*	#include <postscreen.h>
/*
/*	void	psc_acl_pre_jail_init()
/*
/*	ARGV	*psc_acl_parse(raw_acl, origin)
/*	const char *raw_acl;
/*	const char *origin;
/*
/*	int	psc_acl_eval(state, cooked_acl, origin)
/*	PSC_STATE *state;
/*	ARGV	*cooked_acl;
/*	const char *origin;
/* DESCRIPTION
/*	This module implements the permanent black/whitelist that
/*	is evaluated immediately after a client connects to postscreen.
/*
/*	psc_acl_pre_jail_init() does before-chroot initialization.
/*
/*	psc_acl_parse() converts an access list from raw string
/*	form to binary form.
/*
/*	psc_acl_eval() evaluates an access list for the specified
/*	SMTP session.
/*
/*	Arguments:
/* .IP raw_acl
/*	String with space/comma separated commands.
/* .IP cooked_acl
/*	The parsed access list.
/* .IP origin
/*	This should be "postscreen_access_list" for an access list
/*	from main.cf, and the type:name of a lookup table otherwise.
/*	The information is used for error reporting (nested table,
/*	unknown keyword).
/* .IP state
/*	Connection state.
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

/* Global library. */

#include <mail_params.h>
#include <addr_match_list.h>
#include <match_parent_style.h>

/* Application-specific. */

#include <postscreen.h>

#define PSC_ACL_SEPARATORS	", \t\r"

static ADDR_MATCH_LIST *psc_mynetworks;

/* psc_acl_pre_jail_init - initialize */

void    psc_acl_pre_jail_init(void)
{
    if (psc_mynetworks)
	addr_match_list_free(psc_mynetworks);
    psc_mynetworks = addr_match_list_init(match_parent_style(VAR_MYNETWORKS),
					  var_mynetworks);
}

/* psc_acl_parse - parse access list */

ARGV   *psc_acl_parse(const char *acl, const char *origin)
{
    char   *saved_checks = mystrdup(acl);
    ARGV   *argv = argv_alloc(1);
    char   *bp = saved_checks;
    char   *name;

#define STREQ(x,y) (strcasecmp((x), (y)) == 0)
#define STRNE(x,y) (strcasecmp((x), (y)) != 0)

    /*
     * Nested tables are not allowed. Tables are opened before entering the
     * chroot jail, while access lists are evaluated after entering the
     * chroot jail.
     */
    while ((name = mystrtok(&bp, PSC_ACL_SEPARATORS)) != 0) {
	if (strchr(name, ':') != 0) {
	    if (STRNE(origin, VAR_PSC_ACL)) {
		msg_warn("table %s: lookup result \"%s\" is not allowed"
			 " -- ignoring remainder of access list",
			 origin, name);
		argv_add(argv, PSC_ACL_NAME_DUNNO, (char *) 0);
		break;
	    } else {
		if (dict_handle(name) == 0)
		    dict_register(name, dict_open(name, O_RDONLY, DICT_FLAG_LOCK
						  | DICT_FLAG_FOLD_FIX));
	    }
	}
	argv_add(argv, name, (char *) 0);
    }
    argv_terminate(argv);

    /*
     * Cleanup.
     */
    myfree(saved_checks);
    return (argv);
}

/* psc_acl_eval - evaluate access list */

int     psc_acl_eval(PSC_STATE *state, ARGV *acl, const char *origin)
{
    const char *myname = "psc_acl_eval";
    char  **cpp;
    DICT   *dict;
    ARGV   *argv;
    const char *name;
    const char *dict_val;
    int     ret;

    for (cpp = acl->argv; (name = *cpp) != 0; cpp++) {
	if (msg_verbose)
	    msg_info("source=%s address=%s acl=%s",
		     origin, state->smtp_client_addr, name);
	if (STREQ(name, PSC_ACL_NAME_BLACKLIST)) {
	    return (PSC_ACL_ACT_BLACKLIST);
	} else if (STREQ(name, PSC_ACL_NAME_WHITELIST)) {
	    return (PSC_ACL_ACT_WHITELIST);
	} else if (STREQ(name, PSC_ACL_NAME_WL_MYNETWORKS)) {
	    if (addr_match_list_match(psc_mynetworks, state->smtp_client_addr))
		return (PSC_ACL_ACT_WHITELIST);
	} else if (strchr(name, ':') != 0) {
	    if ((dict = dict_handle(name)) == 0)
		msg_panic("%s: unexpected dictionary: %s", myname, name);
	    if ((dict_val = dict_get(dict, state->smtp_client_addr)) != 0) {
		argv = psc_acl_parse(dict_val, name);
		ret = psc_acl_eval(state, argv, name);
		argv_free(argv);
		if (ret != PSC_ACL_ACT_DUNNO)
		    return (ret);
	    } else if (dict_errno != 0) {
		msg_warn("%s: table lookup error -- ignoring the remainder "
			 "of this access list", name);
		return (PSC_ACL_ACT_ERROR);
	    }
	} else if (STREQ(name, PSC_ACL_NAME_DUNNO)) {
	    return (PSC_ACL_ACT_DUNNO);
	} else {
	    msg_warn("%s: unknown command: %s -- ignoring the remainder "
		     "of this access list", origin, name);
	    return (PSC_ACL_ACT_ERROR);
	}
    }
    if (msg_verbose)
	msg_info("source=%s address=%s - no match",
		 origin, state->smtp_client_addr);
    return (PSC_ACL_ACT_DUNNO);
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
char   *var_psc_acl = "";

#define UPDATE_VAR(s,v) do { if (*(s)) myfree(s); (s) = mystrdup(v); } while (0)

int     main(void)
{
    VSTRING *buf = vstring_alloc(100);
    PSC_STATE state;
    ARGV   *argv;
    int     ret;
    int     have_tty = isatty(0);
    char   *bufp;
    char   *cmd;
    char   *value;
    const NAME_CODE acl_map[] = {
	PSC_ACL_NAME_ERROR, PSC_ACL_ACT_ERROR,
	PSC_ACL_NAME_WHITELIST, PSC_ACL_ACT_WHITELIST,
	PSC_ACL_NAME_BLACKLIST, PSC_ACL_ACT_BLACKLIST,
	PSC_ACL_NAME_DUNNO, PSC_ACL_ACT_DUNNO,
	0,
    };

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
			   VAR_MYNETWORKS, VAR_PSC_ACL);
	} else if ((value = mystrtok(&bufp, " =")) == 0) {
	    vstream_printf("missing value\n");
	} else if (STREQ(cmd, VAR_MYNETWORKS)) {
	    UPDATE_VAR(var_mynetworks, value);
	} else if (STREQ(cmd, VAR_PSC_ACL)) {
	    UPDATE_VAR(var_psc_acl, value);
	} else if (STREQ(cmd, "address")) {
	    psc_acl_pre_jail_init();
	    argv = psc_acl_parse(var_psc_acl, VAR_PSC_ACL);
	    state.smtp_client_addr = value;
	    ret = psc_acl_eval(&state, argv, VAR_PSC_ACL);
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
