/*	$NetBSD: login_sender_match.c,v 1.2 2022/10/08 16:12:45 christos Exp $	*/

/*++
/* NAME
/*	login_sender_match 3
/* SUMMARY
/*	match login and sender against (login, sender) patterns
/* SYNOPSIS
/*	#include <login_sender.h>
/*
/*	typedef LOGIN_SENDER_MATCH LOGIN_SENDER_MATCH;
/*
/*	LOGIN_SENDER_MATCH *login_sender_create(
/*	const char *title,
/*	const char *map_names,
/*	const char *ext_delimiters,
/*	const char *null_sender,
/*	const char *wildcard)
/*
/*	void	login_sender_free(
/*	LOGIN_SENDER_MATCH *lsm)
/*
/*	int	login_sender_match(
/*	LOGIN_SENDER_MATCH *lsm,
/*	const char *login_name,
/*	const char *sender_addr)
/* DESCRIPTION
/*	This module determines if a login name and internal-form
/*	sender address match a (login name, external-form sender
/*	patterns) table entry. A login name matches if it matches
/*	a lookup table key. A sender address matches the corresponding
/*	table entry if it matches a sender pattern. A wildcard
/*	sender pattern matches any sender address. A sender pattern
/*	that starts with '@' matches the '@' and the domain portion
/*	of a sender address. Otherwise, the matcher ignores the
/*	extension part of a sender address, and requires a
/*	case-insensitive match against a sender pattern.
/*
/*	login_sender_create() creates a (login name, sender patterns)
/*	matcher.
/*
/*	login_sender_free() destroys the specified (login name,
/*	sender patterns) matcher.
/*
/*	login_sender_match() looks up an entry for the \fBlogin_name\fR
/*	argument, and determines if the lookup result matches the
/*	\fBsender_adddr\fR argument.
/*
/*	Arguments:
/* .IP title
/*	The name of the configuration parameter that specifies the
/*	map_names value, used for error messages.
/* .IP map_names
r*	The lookup table(s) with (login name, sender patterns) entries.
/* .IP ext_delimiters
/*	The set of address extension delimiters.
/* .IP null_sender
/*	If a sender pattern equals the null_sender pattern, then
/*	the empty address is matched.
/* .IP wildcard
/*	Null pointer, or non-empty string with a wildcard pattern.
/*	If a sender pattern equals the wildcard pattern, then any
/*	sender address is matched.
/* .IP login_name
/*	The login name (for example, UNIX account, or SASL username)
/*	that will be used as a search key to locate a list of senders.
/* .IP sender_addr
/*	The sender email address (unquoted form) that will be matched
/*	against a (login name, sender patterns) table entry.
/* DIAGNOSTICS
/*	login_sender_match() returns LSM_STAT_FOUND if a
/*	match was found, LOGIN_STAT_NOTFOUND if no match was found,
/*	LSM_STAT_RETRY if the table lookup failed, or
/*	LSM_STAT_CONFIG in case of a configuration error.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <vstring.h>

 /*
  * Global library.
  */
#include <mail_params.h>
#include <maps.h>
#include <quote_822_local.h>
#include <strip_addr.h>
#include <login_sender_match.h>

 /*
  * Private data structure.
  */
struct LOGIN_SENDER_MATCH {
    MAPS   *maps;
    VSTRING *ext_stripped_sender;
    char   *ext_delimiters;
    char   *null_sender;
    char   *wildcard;
};

 /*
  * SLMs.
  */
#define STR(x)	vstring_str(x)

/* login_sender_create - create (login name, sender patterns) matcher */

LOGIN_SENDER_MATCH *login_sender_create(const char *title,
					        const char *map_names,
					        const char *ext_delimiters,
					        const char *null_sender,
					        const char *wildcard)
{
    LOGIN_SENDER_MATCH *lsm = mymalloc(sizeof *lsm);

    lsm->maps = maps_create(title, map_names, DICT_FLAG_LOCK
			    | DICT_FLAG_FOLD_FIX | DICT_FLAG_UTF8_REQUEST);
    lsm->ext_stripped_sender = vstring_alloc(100);
    lsm->ext_delimiters = mystrdup(ext_delimiters);
    if (null_sender == 0 || *null_sender == 0)
	msg_panic("login_sender_create: null or empty null_sender");
    lsm->null_sender = mystrdup(null_sender);
    lsm->wildcard = (wildcard && *wildcard) ? mystrdup(wildcard) : 0;
    return (lsm);
}

/* login_sender_free - destroy (login name, sender patterns) matcher */

void    login_sender_free(LOGIN_SENDER_MATCH *lsm)
{
    maps_free(lsm->maps);
    vstring_free(lsm->ext_stripped_sender);
    myfree(lsm->ext_delimiters);
    myfree(lsm->null_sender);
    if (lsm->wildcard)
	myfree(lsm->wildcard);
    myfree((void *) lsm);
}

/* strip_externalize_addr - strip address extension and externalize remainder */

static VSTRING *strip_externalize_addr(VSTRING *ext_addr, const char *int_addr,
				               const char *delims)
{
    char   *int_stripped_addr;

    if ((int_stripped_addr = strip_addr_internal(int_addr,
					       /* extension= */ (char **) 0,
						 delims)) != 0) {
	quote_822_local(ext_addr, int_stripped_addr);
	myfree(int_stripped_addr);
	return (ext_addr);
    } else {
	return quote_822_local(ext_addr, int_addr);
    }
}

/* login_sender_match - match login and sender against (login, senders) table */

int     login_sender_match(LOGIN_SENDER_MATCH *lsm, const char *login_name,
			           const char *sender_addr)
{
    int     found_or_error = LSM_STAT_NOTFOUND;

    /* Sender patterns and derived info */
    const char *sender_patterns;
    char   *saved_sender_patterns;
    char   *cp;
    char   *sender_pattern;

    /* Actual sender and derived info */
    const char *ext_stripped_sender = 0;
    const char *at_sender_domain;

    /*
     * Match the login.
     */
    if ((sender_patterns = maps_find(lsm->maps, login_name,
				      /* flags= */ 0)) != 0) {

	/*
	 * Match the sender. Don't break a sender pattern between double
	 * quotes.
	 */
	cp = saved_sender_patterns = mystrdup(sender_patterns);
	while (found_or_error == LSM_STAT_NOTFOUND
	       && (sender_pattern = mystrtokdq(&cp, CHARS_COMMA_SP)) != 0) {
	    /* Special pattern: @domain. */
	    if (*sender_pattern == '@') {
		if ((at_sender_domain = strrchr(sender_addr, '@')) != 0
		  && strcasecmp_utf8(sender_pattern, at_sender_domain) == 0)
		    found_or_error = LSM_STAT_FOUND;
	    }
	    /* Special pattern: wildcard. */
	    else if (strcasecmp(sender_pattern, lsm->wildcard) == 0) {
		found_or_error = LSM_STAT_FOUND;
	    }
	    /* Special pattern: empty sender. */
	    else if (strcasecmp(sender_pattern, lsm->null_sender) == 0) {
		if (*sender_addr == 0)
		    found_or_error = LSM_STAT_FOUND;
	    }
	    /* Literal pattern: match the stripped and externalized sender. */
	    else {
		if (ext_stripped_sender == 0)
		    ext_stripped_sender =
			STR(strip_externalize_addr(lsm->ext_stripped_sender,
						   sender_addr,
						   lsm->ext_delimiters));
		if (strcasecmp_utf8(sender_pattern, ext_stripped_sender) == 0)
		    found_or_error = LSM_STAT_FOUND;
	    }
	}
	myfree(saved_sender_patterns);
    } else {
	found_or_error = lsm->maps->error;
    }
    return (found_or_error);
}

#ifdef TEST

int     main(int argc, char **argv)
{
    struct testcase {
	const char *title;
	const char *map_names;
	const char *ext_delimiters;
	const char *null_sender;
	const char *wildcard;
	const char *login_name;
	const char *sender_addr;
	int     exp_return;
    };
    struct testcase testcases[] = {
	{"wildcard works",
	    "inline:{root=*, {foo = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "root", "anything", LSM_STAT_FOUND
	},
	{"unknown user",
	    "inline:{root=*, {foo = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "toor", "anything", LSM_STAT_NOTFOUND
	},
	{"bare user",
	    "inline:{root=*, {foo = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "foo", LSM_STAT_FOUND
	},
	{"user@domain",
	    "inline:{root=*, {foo = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "foo@example.com", LSM_STAT_FOUND
	},
	{"user+ext@domain",
	    "inline:{root=*, {foo = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "foo+bar@example.com", LSM_STAT_FOUND
	},
	{"wrong sender",
	    "inline:{root=*, {foo = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "bar@example.com", LSM_STAT_NOTFOUND
	},
	{"@domain",
	    "inline:{root=*, {foo = @example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "anyone@example.com", LSM_STAT_FOUND
	},
	{"wrong @domain",
	    "inline:{root=*, {foo = @example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "anyone@example.org", LSM_STAT_NOTFOUND
	},
	{"null sender",
	    "inline:{root=*, {foo = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "bar", "", LSM_STAT_FOUND
	},
	{"wrong null sender",
	    "inline:{root=*, {foo = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "baz", "", LSM_STAT_NOTFOUND
	},
	{"error",
	    "inline:{root=*}, fail:sorry",
	    "+-", "<>", "*", "baz", "whatever", LSM_STAT_RETRY
	},
	{"no error",
	    "inline:{root=*}, fail:sorry",
	    "+-", "<>", "*", "root", "whatever", LSM_STAT_FOUND
	},
	{"unknown uid:number",
	    "inline:{root=*, {uid:12345 = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "uid:54321", "foo", LSM_STAT_NOTFOUND
	},
	{"known uid:number",
	    "inline:{root=*, {uid:12345 = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "uid:12345", "foo", LSM_STAT_FOUND
	},
	{"unknown \"other last\"",
	    "inline:{root=*, {foo = \"first last\",\"first last\"@example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "other last", LSM_STAT_NOTFOUND
	},
	{"bare \"first last\"",
	    "inline:{root=*, {foo = \"first last\",\"first last\"@example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "first last", LSM_STAT_FOUND
	},
	{"\"first last\"@domain",
	    "inline:{root=*, {foo = \"first last\",\"first last\"@example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "first last@example.com", LSM_STAT_FOUND
	},
    };
    struct testcase *tp;
    int     act_return;
    int     pass;
    int     fail;
    LOGIN_SENDER_MATCH *lsm;

    /*
     * Fake variable settings.
     */
    var_double_bounce_sender = DEF_DOUBLE_BOUNCE;
    var_ownreq_special = DEF_OWNREQ_SPECIAL;

#define NUM_TESTS	sizeof(testcases)/sizeof(testcases[0])

    for (pass = fail = 0, tp = testcases; tp < testcases + NUM_TESTS; tp++) {
	msg_info("RUN test case %ld %s", (long) (tp - testcases), tp->title);
#if 0
	msg_info("title=%s", tp->title);
	msg_info("map_names=%s", tp->map_names);
	msg_info("ext_delimiters=%s", tp->ext_delimiters);
	msg_info("null_sender=%s", tp->null_sender);
	msg_info("wildcard=%s", tp->wildcard);
	msg_info("login_name=%s", tp->login_name);
	msg_info("sender_addr=%s", tp->sender_addr);
	msg_info("exp_return=%d", tp->exp_return);
#endif
	lsm = login_sender_create("test map", tp->map_names,
				  tp->ext_delimiters, tp->null_sender,
				  tp->wildcard);
	act_return = login_sender_match(lsm, tp->login_name, tp->sender_addr);
	if (act_return == tp->exp_return) {
	    msg_info("PASS test %ld", (long) (tp - testcases));
	    pass++;
	} else {
	    msg_info("FAIL test %ld", (long) (tp - testcases));
	    fail++;
	}
	login_sender_free(lsm);
    }
    return (fail > 0);
}

#endif					/* TEST */
