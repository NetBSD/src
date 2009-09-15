/*	$NetBSD: mail_conf_long.c,v 1.1.1.1.2.2 2009/09/15 06:02:45 snj Exp $	*/

/*++
/* NAME
/*	mail_conf_long 3
/* SUMMARY
/*	long integer-valued configuration parameter support
/* SYNOPSIS
/*	#include <mail_conf.h>
/*
/*	int	get_mail_conf_long(name, defval, min, max);
/*	const char *name;
/*	long	defval;
/*	long	min;
/*	long	max;
/*
/*	int	get_mail_conf_long_fn(name, defval, min, max);
/*	const char *name;
/*	long	(*defval)(void);
/*	long	min;
/*	long	max;
/*
/*	void	set_mail_conf_long(name, value)
/*	const char *name;
/*	long	value;
/*
/*	void	get_mail_conf_long_table(table)
/*	const CONFIG_LONG_TABLE *table;
/*
/*	void	get_mail_conf_long_fn_table(table)
/*	const CONFIG_LONG_TABLE *table;
/* AUXILIARY FUNCTIONS
/*	int	get_mail_conf_long2(name1, name2, defval, min, max);
/*	const char *name1;
/*	const char *name2;
/*	long	defval;
/*	long	min;
/*	long	max;
/* DESCRIPTION
/*	This module implements configuration parameter support
/*	for long integer values.
/*
/*	get_mail_conf_long() looks up the named entry in the global
/*	configuration dictionary. The default value is returned
/*	when no value was found.
/*	\fImin\fR is zero or specifies a lower limit on the long
/*	integer value; \fImax\fR is zero or specifies an upper limit
/*	on the long integer value.
/*
/*	get_mail_conf_long_fn() is similar but specifies a function that
/*	provides the default value. The function is called only
/*	when the default value is needed.
/*
/*	set_mail_conf_long() updates the named entry in the global
/*	configuration dictionary. This has no effect on values that
/*	have been looked up earlier via the get_mail_conf_XXX() routines.
/*
/*	get_mail_conf_long_table() and get_mail_conf_long_fn_table() initialize
/*	lists of variables, as directed by their table arguments. A table
/*	must be terminated by a null entry.
/*
/*	get_mail_conf_long2() concatenates the two names and is otherwise
/*	identical to get_mail_conf_long().
/* DIAGNOSTICS
/*	Fatal errors: malformed numerical value.
/* SEE ALSO
/*	config(3) general configuration
/*	mail_conf_str(3) string-valued configuration parameters
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
#include <stdio.h>			/* sscanf() */

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <dict.h>
#include <stringops.h>

/* Global library. */

#include "mail_conf.h"

/* convert_mail_conf_long - look up and convert integer parameter value */

static int convert_mail_conf_long(const char *name, long *longval)
{
    const char *strval;
    char    junk;

    if ((strval = mail_conf_lookup_eval(name)) != 0) {
	if (sscanf(strval, "%ld%c", longval, &junk) != 1)
	    msg_fatal("bad numerical configuration: %s = %s", name, strval);
	return (1);
    }
    return (0);
}

/* check_mail_conf_long - validate integer value */

static void check_mail_conf_long(const char *name, long longval, long min, long max)
{
    if (min && longval < min)
	msg_fatal("invalid %s parameter value %ld < %ld", name, longval, min);
    if (max && longval > max)
	msg_fatal("invalid %s parameter value %ld > %ld", name, longval, max);
}

/* get_mail_conf_long - evaluate integer-valued configuration variable */

long    get_mail_conf_long(const char *name, long defval, long min, long max)
{
    long    longval;

    if (convert_mail_conf_long(name, &longval) == 0)
	set_mail_conf_long(name, longval = defval);
    check_mail_conf_long(name, longval, min, max);
    return (longval);
}

/* get_mail_conf_long2 - evaluate integer-valued configuration variable */

long    get_mail_conf_long2(const char *name1, const char *name2, long defval,
			            long min, long max)
{
    long    longval;
    char   *name;

    name = concatenate(name1, name2, (char *) 0);
    if (convert_mail_conf_long(name, &longval) == 0)
	set_mail_conf_long(name, longval = defval);
    check_mail_conf_long(name, longval, min, max);
    myfree(name);
    return (longval);
}

/* get_mail_conf_long_fn - evaluate integer-valued configuration variable */

typedef long (*stupid_indent_long) (void);

long    get_mail_conf_long_fn(const char *name, stupid_indent_long defval,
			              long min, long max)
{
    long    longval;

    if (convert_mail_conf_long(name, &longval) == 0)
	set_mail_conf_long(name, longval = defval());
    check_mail_conf_long(name, longval, min, max);
    return (longval);
}

/* set_mail_conf_long - update integer-valued configuration dictionary entry */

void    set_mail_conf_long(const char *name, long value)
{
    char    buf[BUFSIZ];		/* yeah! crappy code! */

    sprintf(buf, "%ld", value);			/* yeah! more crappy code! */
    mail_conf_update(name, buf);
}

/* get_mail_conf_long_table - look up table of integers */

void    get_mail_conf_long_table(const CONFIG_LONG_TABLE *table)
{
    while (table->name) {
	table->target[0] = get_mail_conf_long(table->name, table->defval,
					      table->min, table->max);
	table++;
    }
}

/* get_mail_conf_long_fn_table - look up integers, defaults are functions */

void    get_mail_conf_long_fn_table(const CONFIG_LONG_FN_TABLE *table)
{
    while (table->name) {
	table->target[0] = get_mail_conf_long_fn(table->name, table->defval,
						 table->min, table->max);
	table++;
    }
}
