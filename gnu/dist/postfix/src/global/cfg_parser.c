/*++
/* NAME
/*	cfg_parser 3
/* SUMMARY
/*	configuration parser utilities
/* SYNOPSIS
/*	#include "cfg_parser.h"
/*
/*	CFG_PARSER *cfg_parser_alloc(pname)
/*	const char *pname;
/*
/*	CFG_PARSER *cfg_parser_free(parser)
/*	CFG_PARSER *parser;
/*
/*	char *cfg_get_str(parser, name, defval, min, max)
/*	const CFG_PARSER *parser;
/*	const char *name;
/*	const char *defval;
/*	int min;
/*	int max;
/*
/*	int cfg_get_int(parser, name, defval, min, max)
/*	const CFG_PARSER *parser;
/*	const char *name;
/*	int defval;
/*	int min;
/*	int max;
/*
/*	int cfg_get_bool(parser, name, defval)
/*	const CFG_PARSER *parser;
/*	const char *name;
/*	int defval;
/* DESCRIPTION
/*	This module implements utilities for parsing parameters defined
/*	either as "\fIname\fR = \fBvalue\fR" in a file pointed to by
/*	\fIpname\fR (the old MySQL style), or as "\fIpname\fR_\fIname\fR =
/*	\fBvalue\fR" in main.cf (the old LDAP style).  It unifies the
/*	two styles and provides support for range checking.
/*
/*	\fIcfg_parser_alloc\fR initializes the parser.
/*
/*	\fIcfg_parser_free\fR releases the parser.
/*
/*	\fIcfg_get_str\fR looks up a string.
/*
/*	\fIcfg_get_int\fR looks up an integer.
/*
/*	\fIcfg_get_bool\fR looks up a boolean value.
/*
/*	\fIdefval\fR is returned when no value was found. \fImin\fR is
/*	zero or specifies a lower limit on the integer value or string
/*	length; \fImax\fR is zero or specifies an upper limit on the
/*	integer value or string length.
/*
/*	Conveniently, \fIcfg_get_str\fR returns \fBNULL\fR if
/*	\fIdefval\fR is \fBNULL\fR and no value was found.  The returned
/*	string has to be freed by the caller if not \fBNULL\fR.
/* DIAGNOSTICS
/*	Fatal errors: bad string length, malformed numerical value, malformed
/*	boolean value.
/* SEE ALSO
/*	mail_conf_str(3) string-valued global configuration parameter support
/*	mail_conf_int(3) integer-valued configuration parameter support
/*	mail_conf_bool(3) boolean-valued configuration parameter support
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
/*	Liviu Daia
/*	Institute of Mathematics of the Romanian Academy
/*	P.O. BOX 1-764
/*	RO-014700 Bucharest, ROMANIA
/*--*/

/* System library. */

#include "sys_defs.h"

#include <stdio.h>
#include <string.h>

/* Utility library. */

#include "msg.h"
#include "mymalloc.h"
#include "vstring.h"
#include "dict.h"

/* Global library. */

#include "mail_conf.h"

/* Application-specific. */

#include "cfg_parser.h"

/* get string from file */

static char *get_dict_str(const struct CFG_PARSER *parser,
			          const char *name, const char *defval,
			          int min, int max)
{
    const char *strval;
    int     len;

    if ((strval = (char *) dict_lookup(parser->name, name)) == 0)
	strval = defval;

    len = strlen(strval);
    if (min && len < min)
	msg_fatal("%s: bad string length %d < %d: %s = %s",
		  parser->name, len, min, name, strval);
    if (max && len > max)
	msg_fatal("%s: bad string length %d > %d: %s = %s",
		  parser->name, len, max, name, strval);
    return (mystrdup(strval));
}

/* get string from main.cf */

static char *get_main_str(const struct CFG_PARSER *parser,
			          const char *name, const char *defval,
			          int min, int max)
{
    static VSTRING *buf = 0;

    if (buf == 0)
	buf = vstring_alloc(15);
    vstring_sprintf(buf, "%s_%s", parser->name, name);
    return ((char *) get_mail_conf_str(vstring_str(buf), defval, min, max));
}

/* get integer from file */

static int get_dict_int(const struct CFG_PARSER *parser,
		             const char *name, int defval, int min, int max)
{
    const char *strval;
    int     intval;
    char    junk;

    if ((strval = (char *) dict_lookup(parser->name, name)) != 0) {
	if (sscanf(strval, "%d%c", &intval, &junk) != 1)
	    msg_fatal("%s: bad numerical configuration: %s = %s",
		      parser->name, name, strval);
    } else
	intval = defval;
    if (min && intval < min)
	msg_fatal("%s: invalid %s parameter value %d < %d",
		  parser->name, name, intval, min);
    if (max && intval > max)
	msg_fatal("%s: invalid %s parameter value %d > %d",
		  parser->name, name, intval, max);
    return (intval);
}

/* get integer from main.cf */

static int get_main_int(const struct CFG_PARSER *parser,
		             const char *name, int defval, int min, int max)
{
    static VSTRING *buf = 0;

    if (buf == 0)
	buf = vstring_alloc(15);
    vstring_sprintf(buf, "%s_%s", parser->name, name);
    return (get_mail_conf_int(vstring_str(buf), defval, min, max));
}

/* get boolean option from file */

static int get_dict_bool(const struct CFG_PARSER *parser,
			         const char *name, int defval)
{
    const char *strval;
    int     intval;

    if ((strval = (char *) dict_lookup(parser->name, name)) != 0) {
	if (strcasecmp(strval, CONFIG_BOOL_YES) == 0) {
	    intval = 1;
	} else if (strcasecmp(strval, CONFIG_BOOL_NO) == 0) {
	    intval = 0;
	} else {
	    msg_fatal("%s: bad boolean configuration: %s = %s",
		      parser->name, name, strval);
	}
    } else
	intval = defval;
    return (intval);
}

/* get boolean option from main.cf */

static int get_main_bool(const struct CFG_PARSER *parser,
			         const char *name, int defval)
{
    static VSTRING *buf = 0;

    if (buf == 0)
	buf = vstring_alloc(15);
    vstring_sprintf(buf, "%s_%s", parser->name, name);
    return (get_mail_conf_bool(vstring_str(buf), defval));
}

/* initialize parser */

CFG_PARSER *cfg_parser_alloc(const char *pname)
{
    const char *myname = "cfg_parser_alloc";
    CFG_PARSER *parser;

    if (pname == 0 || *pname == 0)
	msg_fatal("%s: null parser name", myname);
    parser = (CFG_PARSER *) mymalloc(sizeof(*parser));
    parser->name = mystrdup(pname);
    if (*parser->name == '/' || *parser->name == '.') {
	dict_load_file(parser->name, parser->name);
	parser->get_str = get_dict_str;
	parser->get_int = get_dict_int;
	parser->get_bool = get_dict_bool;
    } else {
	parser->get_str = get_main_str;
	parser->get_int = get_main_int;
	parser->get_bool = get_main_bool;
    }
    return (parser);
}

/* get string */

char   *cfg_get_str(const CFG_PARSER *parser, const char *name,
		            const char *defval, int min, int max)
{
    const char *myname = "cfg_get_str";
    char   *strval;

    strval = parser->get_str(parser, name, (defval ? defval : ""), min, max);
    if (defval == 0 && *strval == 0) {
	/* the caller wants NULL instead of "" */
	myfree(strval);
	strval = 0;
    }
    if (msg_verbose)
	msg_info("%s: %s: %s = %s", myname, parser->name, name,
		 (strval ? strval : "<NULL>"));
    return (strval);
}

/* get integer */

int     cfg_get_int(const CFG_PARSER *parser, const char *name, int defval,
		            int min, int max)
{
    const char *myname = "cfg_get_int";
    int     intval;

    intval = parser->get_int(parser, name, defval, min, max);
    if (msg_verbose)
	msg_info("%s: %s: %s = %d", myname, parser->name, name, intval);
    return (intval);
}

/* get boolean option */

int     cfg_get_bool(const CFG_PARSER *parser, const char *name, int defval)
{
    const char *myname = "cfg_get_bool";
    int     intval;

    intval = parser->get_bool(parser, name, defval);
    if (msg_verbose)
	msg_info("%s: %s: %s = %s", myname, parser->name, name,
		 (intval ? "on" : "off"));
    return (intval);
}

/* release parser */

CFG_PARSER *cfg_parser_free(CFG_PARSER *parser)
{
    const char *myname = "cfg_parser_free";

    if (parser->name == 0 || *parser->name == 0)
	msg_panic("%s: null parser name", myname);
    if (*parser->name == '/' || *parser->name == '.') {
	if (dict_handle(parser->name))
	    dict_unregister(parser->name);
    }
    myfree(parser->name);
    myfree((char *) parser);
    return (0);
}
