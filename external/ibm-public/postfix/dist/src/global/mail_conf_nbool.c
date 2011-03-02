/*	$NetBSD: mail_conf_nbool.c,v 1.1.1.1 2011/03/02 19:32:15 tron Exp $	*/

/*++
/* NAME
/*	mail_conf_nbool 3
/* SUMMARY
/*	boolean-valued configuration parameter support
/* SYNOPSIS
/*	#include <mail_conf.h>
/*
/*	int	get_mail_conf_nbool(name, defval)
/*	const char *name;
/*	const char *defval;
/*
/*	int	get_mail_conf_nbool_fn(name, defval)
/*	const char *name;
/*	const char *(*defval)();
/*
/*	void	set_mail_conf_nbool(name, value)
/*	const char *name;
/*	const char *value;
/*
/*	void	get_mail_conf_nbool_table(table)
/*	const CONFIG_NBOOL_TABLE *table;
/*
/*	void	get_mail_conf_nbool_fn_table(table)
/*	const CONFIG_NBOOL_TABLE *table;
/* DESCRIPTION
/*	This module implements configuration parameter support for
/*	boolean values. The internal representation is zero (false)
/*	and non-zero (true). The external representation is "no"
/*	(false) and "yes" (true). The conversion from external
/*	representation is case insensitive. Unlike mail_conf_bool(3)
/*	the default is a string value which is subject to macro expansion.
/*
/*	get_mail_conf_nbool() looks up the named entry in the global
/*	configuration dictionary. The specified default value is
/*	returned when no value was found.
/*
/*	get_mail_conf_nbool_fn() is similar but specifies a function that
/*	provides the default value. The function is called only
/*	when the default value is needed.
/*
/*	set_mail_conf_nbool() updates the named entry in the global
/*	configuration dictionary. This has no effect on values that
/*	have been looked up earlier via the get_mail_conf_XXX() routines.
/*
/*	get_mail_conf_nbool_table() and get_mail_conf_int_fn_table() initialize
/*	lists of variables, as directed by their table arguments. A table
/*	must be terminated by a null entry.
/* DIAGNOSTICS
/*	Fatal errors: malformed boolean value.
/* SEE ALSO
/*	config(3) general configuration
/*	mail_conf_str(3) string-valued configuration parameters
/*	mail_conf_int(3) integer-valued configuration parameters
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

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <dict.h>

/* Global library. */

#include "mail_conf.h"

/* convert_mail_conf_nbool - look up and convert boolean parameter value */

static int convert_mail_conf_nbool(const char *name, int *intval)
{
    const char *strval;

    if ((strval = mail_conf_lookup_eval(name)) == 0) {
	return (0);
    } else {
	if (strcasecmp(strval, CONFIG_BOOL_YES) == 0) {
	    *intval = 1;
	} else if (strcasecmp(strval, CONFIG_BOOL_NO) == 0) {
	    *intval = 0;
	} else {
	    msg_fatal("bad boolean configuration: %s = %s", name, strval);
	}
	return (1);
    }
}

/* get_mail_conf_nbool - evaluate boolean-valued configuration variable */

int     get_mail_conf_nbool(const char *name, const char *defval)
{
    int     intval;

    if (convert_mail_conf_nbool(name, &intval) == 0)
	set_mail_conf_nbool(name, defval);
    if (convert_mail_conf_nbool(name, &intval) == 0)
	msg_panic("get_mail_conf_nbool: parameter not found: %s", name);
    return (intval);
}

/* get_mail_conf_nbool_fn - evaluate boolean-valued configuration variable */

typedef const char *(*stupid_indent_int) (void);

int     get_mail_conf_nbool_fn(const char *name, stupid_indent_int defval)
{
    int     intval;

    if (convert_mail_conf_nbool(name, &intval) == 0)
	set_mail_conf_nbool(name, defval());
    if (convert_mail_conf_nbool(name, &intval) == 0)
	msg_panic("get_mail_conf_nbool_fn: parameter not found: %s", name);
    return (intval);
}

/* set_mail_conf_nbool - update boolean-valued configuration dictionary entry */

void    set_mail_conf_nbool(const char *name, const char *value)
{
    mail_conf_update(name, value);
}

/* get_mail_conf_nbool_table - look up table of booleans */

void    get_mail_conf_nbool_table(const CONFIG_NBOOL_TABLE *table)
{
    while (table->name) {
	table->target[0] = get_mail_conf_nbool(table->name, table->defval);
	table++;
    }
}

/* get_mail_conf_nbool_fn_table - look up booleans, defaults are functions */

void    get_mail_conf_nbool_fn_table(const CONFIG_NBOOL_FN_TABLE *table)
{
    while (table->name) {
	table->target[0] = get_mail_conf_nbool_fn(table->name, table->defval);
	table++;
    }
}
