/*++
/* NAME
/*	mail_conf_int 3
/* SUMMARY
/*	integer-valued configuration parameter support
/* SYNOPSIS
/*	#include <mail_conf.h>
/*
/*	int	get_mail_conf_int(name, defval, min, max);
/*	const char *name;
/*	int	defval;
/*	int	min;
/*	int	max;
/*
/*	int	get_mail_conf_int_fn(name, defval, min, max);
/*	const char *name;
/*	int	(*defval)();
/*	int	min;
/*	int	max;
/*
/*	void	set_mail_conf_int(name, value)
/*	const char *name;
/*	int	value;
/*
/*	void	get_mail_conf_int_table(table)
/*	CONFIG_INT_TABLE *table;
/*
/*	void	get_mail_conf_int_fn_table(table)
/*	CONFIG_INT_TABLE *table;
/* AUXILIARY FUNCTIONS
/*	int	get_mail_conf_int2(name1, name2, defval, min, max);
/*	const char *name1;
/*	const char *name2;
/*	int	defval;
/*	int	min;
/*	int	max;
/* DESCRIPTION
/*	This module implements configuration parameter support
/*	for integer values.
/*
/*	get_mail_conf_int() looks up the named entry in the global
/*	configuration dictionary. The default value is returned
/*	when no value was found.
/*	\fImin\fR is zero or specifies a lower limit on the integer
/*	value or string length; \fImax\fR is zero or specifies an
/*	upper limit on the integer value or string length.
/*
/*	get_mail_conf_int_fn() is similar but specifies a function that
/*	provides the default value. The function is called only
/*	when the default value is needed.
/*
/*	set_mail_conf_int() updates the named entry in the global
/*	configuration dictionary. This has no effect on values that
/*	have been looked up earlier via the get_mail_conf_XXX() routines.
/*
/*	get_mail_conf_int_table() and get_mail_conf_int_fn_table() initialize
/*	lists of variables, as directed by their table arguments. A table
/*	must be terminated by a null entry.
/*
/*	get_mail_conf_int2() concatenates the two names and is otherwise
/*	identical to get_mail_conf_int().
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

/* convert_mail_conf_int - look up and convert integer parameter value */

static int convert_mail_conf_int(const char *name, int *intval)
{
    const char *strval;
    char    junk;

    if ((strval = mail_conf_lookup_eval(name)) != 0) {
	if (sscanf(strval, "%d%c", intval, &junk) != 1)
	    msg_fatal("bad numerical configuration: %s = %s", name, strval);
	return (1);
    }
    return (0);
}

/* check_mail_conf_int - validate integer value */

static void check_mail_conf_int(const char *name, int intval, int min, int max)
{
    if (min && intval < min)
	msg_fatal("invalid %s parameter value %d < %d", name, intval, min);
    if (max && intval > max)
	msg_fatal("invalid %s parameter value %d > %d", name, intval, max);
}

/* get_mail_conf_int - evaluate integer-valued configuration variable */

int     get_mail_conf_int(const char *name, int defval, int min, int max)
{
    int     intval;

    if (convert_mail_conf_int(name, &intval) == 0)
	set_mail_conf_int(name, intval = defval);
    check_mail_conf_int(name, intval, min, max);
    return (intval);
}

/* get_mail_conf_int2 - evaluate integer-valued configuration variable */

int     get_mail_conf_int2(const char *name1, const char *name2, int defval,
			        int min, int max)
{
    int     intval;
    char   *name;

    name = concatenate(name1, name2, (char *) 0);
    if (convert_mail_conf_int(name, &intval) == 0)
	set_mail_conf_int(name, intval = defval);
    check_mail_conf_int(name, intval, min, max);
    myfree(name);
    return (intval);
}

/* get_mail_conf_int_fn - evaluate integer-valued configuration variable */

typedef int (*stupid_indent_int) (void);

int     get_mail_conf_int_fn(const char *name, stupid_indent_int defval,
			          int min, int max)
{
    int     intval;

    if (convert_mail_conf_int(name, &intval) == 0)
	set_mail_conf_int(name, intval = defval());
    check_mail_conf_int(name, intval, min, max);
    return (intval);
}

/* set_mail_conf_int - update integer-valued configuration dictionary entry */

void    set_mail_conf_int(const char *name, int value)
{
    char    buf[BUFSIZ];		/* yeah! crappy code! */

    sprintf(buf, "%d", value);			/* yeah! more crappy code! */
    mail_conf_update(name, buf);
}

/* get_mail_conf_int_table - look up table of integers */

void    get_mail_conf_int_table(CONFIG_INT_TABLE *table)
{
    while (table->name) {
	table->target[0] = get_mail_conf_int(table->name, table->defval,
					  table->min, table->max);
	table++;
    }
}

/* get_mail_conf_int_fn_table - look up integers, defaults are functions */

void    get_mail_conf_int_fn_table(CONFIG_INT_FN_TABLE *table)
{
    while (table->name) {
	table->target[0] = get_mail_conf_int_fn(table->name, table->defval,
					     table->min, table->max);
	table++;
    }
}
