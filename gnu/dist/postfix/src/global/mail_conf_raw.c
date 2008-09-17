/*++
/* NAME
/*	mail_conf_raw 3
/* SUMMARY
/*	raw string-valued global configuration parameter support
/* SYNOPSIS
/*	#include <mail_conf.h>
/*
/*	char	*get_mail_conf_raw(name, defval, min, max)
/*	const char *name;
/*	const char *defval;
/*	int	min;
/*	int	max;
/*
/*	char	*get_mail_conf_raw_fn(name, defval, min, max)
/*	const char *name;
/*	const char *(*defval)(void);
/*	int	min;
/*	int	max;
/*
/*	void	get_mail_conf_raw_table(table)
/*	const CONFIG_RAW_TABLE *table;
/*
/*	void	get_mail_conf_raw_fn_table(table)
/*	const CONFIG_RAW_TABLE *table;
/* DESCRIPTION
/*	This module implements support for string-valued global
/*	configuration parameters that are loaded without $name expansion.
/*
/*	get_mail_conf_raw() looks up the named entry in the global
/*	configuration dictionary. The default value is returned when
/*	no value was found. String results should be passed to myfree()
/*	when no longer needed.  \fImin\fR is zero or specifies a lower
/*	bound on the string length; \fImax\fR is zero or specifies an
/*	upper limit on the string length.
/*
/*	get_mail_conf_raw_fn() is similar but specifies a function that
/*	provides the default value. The function is called only when
/*	the default value is used.
/*
/*	get_mail_conf_raw_table() and get_mail_conf_raw_fn_table() read
/*	lists of variables, as directed by their table arguments. A table
/*	must be terminated by a null entry.
/* DIAGNOSTICS
/*	Fatal errors: bad string length.
/* SEE ALSO
/*	config(3) generic config parameter support
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
#include <stdlib.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>

/* Global library. */

#include "mail_conf.h"

/* check_mail_conf_raw - validate string length */

static void check_mail_conf_raw(const char *name, const char *strval,
			             int min, int max)
{
    ssize_t len = strlen(strval);

    if (min && len < min)
	msg_fatal("bad string length (%ld < %d): %s = %s",
		  (long) len, min, name, strval);
    if (max && len > max)
	msg_fatal("bad string length (%ld > %d): %s = %s",
		  (long) len, max, name, strval);
}

/* get_mail_conf_raw - evaluate string-valued configuration variable */

char   *get_mail_conf_raw(const char *name, const char *defval,
		               int min, int max)
{
    const char *strval;

    if ((strval = mail_conf_lookup(name)) == 0) {
	strval = defval;
	mail_conf_update(name, strval);
    }
    check_mail_conf_raw(name, strval, min, max);
    return (mystrdup(strval));
}

/* get_mail_conf_raw_fn - evaluate string-valued configuration variable */

typedef const char *(*stupid_indent_str) (void);

char   *get_mail_conf_raw_fn(const char *name, stupid_indent_str defval,
			          int min, int max)
{
    const char *strval;

    if ((strval = mail_conf_lookup(name)) == 0) {
	strval = defval();
	mail_conf_update(name, strval);
    }
    check_mail_conf_raw(name, strval, min, max);
    return (mystrdup(strval));
}

/* get_mail_conf_raw_table - look up table of strings */

void    get_mail_conf_raw_table(const CONFIG_RAW_TABLE *table)
{
    while (table->name) {
	if (table->target[0])
	    myfree(table->target[0]);
	table->target[0] = get_mail_conf_raw(table->name, table->defval,
					  table->min, table->max);
	table++;
    }
}

/* get_mail_conf_raw_fn_table - look up strings, defaults are functions */

void    get_mail_conf_raw_fn_table(const CONFIG_RAW_FN_TABLE *table)
{
    while (table->name) {
	if (table->target[0])
	    myfree(table->target[0]);
	table->target[0] = get_mail_conf_raw_fn(table->name, table->defval,
					     table->min, table->max);
	table++;
    }
}
