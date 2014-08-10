/*	$NetBSD: postconf_match.c,v 1.1.1.1.2.2 2014/08/10 07:12:49 tls Exp $	*/

/*++
/* NAME
/*	postconf_match 3
/* SUMMARY
/*	pattern-matching support
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	int	pcf_parse_field_pattern(field_expr)
/*	char	*field_expr;
/*
/*	const char *pcf_str_field_pattern(field_pattern)
/*	int	field_pattern;
/*
/*	int	pcf_is_magic_field_pattern(field_pattern)
/*	int	field_pattern;
/*
/*	ARGV	*pcf_parse_service_pattern(service_expr, min_expr, max_expr)
/*	const char *service_expr;
/*	int	min_expr;
/*	int	max_expr;
/*
/*	int	PCF_IS_MAGIC_SERVICE_PATTERN(service_pattern)
/*	const ARGV *service_pattern;
/*
/*	int	PCF_MATCH_SERVICE_PATTERN(service_pattern, service_name,
/*					service_type)
/*	const ARGV *service_pattern;
/*	const char *service_name;
/*	const char *service_type;
/*
/*	const char *pcf_str_field_pattern(field_pattern)
/*	int	field_pattern;
/*
/*	int	PCF_IS_MAGIC_PARAM_PATTERN(param_pattern)
/*	const char *param_pattern;
/*
/*	int	PCF_MATCH_PARAM_PATTERN(param_pattern, param_name)
/*	const char *param_pattern;
/*	const char *param_name;
/* DESCRIPTION
/*	pcf_parse_service_pattern() takes an expression and splits
/*	it up on '/' into an array of sub-expressions, This function
/*	returns null if the input does fewer than "min" or more
/*	than "max" sub-expressions.
/*
/*	PCF_IS_MAGIC_SERVICE_PATTERN() returns non-zero if any of
/*	the service name or service type sub-expressions contains
/*	a matching operator (as opposed to string literals that
/*	only match themselves). This is an unsafe macro that evaluates
/*	its arguments more than once.
/*
/*	PCF_MATCH_SERVICE_PATTERN() matches a service name and type
/*	from master.cf against the parsed pattern. This is an unsafe
/*	macro that evaluates its arguments more than once.
/*
/*	pcf_parse_field_pattern() converts a field sub-expression,
/*	and returns the conversion result.
/*
/*	pcf_str_field_pattern() converts a result from
/*	pcf_parse_field_pattern() into string form.
/*
/*	pcf_is_magic_field_pattern() returns non-zero if the field
/*	pattern sub-expression contained a matching operator (as
/*	opposed to a string literal that only matches itself).
/*
/*	PCF_IS_MAGIC_PARAM_PATTERN() returns non-zero if the parameter
/*	sub-expression contains a matching operator (as opposed to
/*	a string literal that only matches itself). This is an
/*	unsafe macro that evaluates its arguments more than once.
/*
/*	PCF_MATCH_PARAM_PATTERN() matches a parameter name from
/*	master.cf against the parsed pattern. This is an unsafe
/*	macro that evaluates its arguments more than once.
/*
/*	Arguments
/* .IP field_expr
/*	A field expression.
/* .IP service_expr
/*	This argument is split on '/' into its constituent
/*	sub-expressions.
/* .IP min_expr
/*	The minimum number of sub-expressions in service_expr.
/* .IP max_expr
/*	The maximum number of sub-expressions in service_expr.
/* .IP service_name
/*	Service name from master.cf.
/* .IP service_type
/*	Service type from master.cf.
/* .IP param_pattern
/*	A parameter name expression.
/* DIAGNOSTICS
/*	Fatal errors: invalid syntax.
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

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>

/* Global library. */

#include <split_at.h>

/* Application-specific. */

#include <postconf.h>

 /*
  * Conversion table. Each PCF_MASTER_NAME_XXX name entry must be stored at
  * table offset PCF_MASTER_FLD_XXX. So don't mess it up.
  */
NAME_CODE pcf_field_name_offset[] = {
    PCF_MASTER_NAME_SERVICE, PCF_MASTER_FLD_SERVICE,
    PCF_MASTER_NAME_TYPE, PCF_MASTER_FLD_TYPE,
    PCF_MASTER_NAME_PRIVATE, PCF_MASTER_FLD_PRIVATE,
    PCF_MASTER_NAME_UNPRIV, PCF_MASTER_FLD_UNPRIV,
    PCF_MASTER_NAME_CHROOT, PCF_MASTER_FLD_CHROOT,
    PCF_MASTER_NAME_WAKEUP, PCF_MASTER_FLD_WAKEUP,
    PCF_MASTER_NAME_MAXPROC, PCF_MASTER_FLD_MAXPROC,
    PCF_MASTER_NAME_CMD, PCF_MASTER_FLD_CMD,
    "*", PCF_MASTER_FLD_WILDC,
    0, PCF_MASTER_FLD_NONE,
};

/* pcf_parse_field_pattern - parse service attribute pattern */

int     pcf_parse_field_pattern(const char *field_name)
{
    int     field_pattern;

    if ((field_pattern = name_code(pcf_field_name_offset,
				   NAME_CODE_FLAG_STRICT_CASE,
				   field_name)) == PCF_MASTER_FLD_NONE)
	msg_fatal("invalid service attribute name: \"%s\"", field_name);
    return (field_pattern);
}

/* pcf_parse_service_pattern - parse service pattern */

ARGV   *pcf_parse_service_pattern(const char *pattern, int min_expr, int max_expr)
{
    ARGV   *argv;
    char  **cpp;

    /*
     * Work around argv_split() lameness.
     */
    if (*pattern == '/')
	return (0);
    argv = argv_split(pattern, PCF_NAMESP_SEP_STR);
    if (argv->argc < min_expr || argv->argc > max_expr) {
	argv_free(argv);
	return (0);
    }

    /*
     * Allow '*' only all by itself.
     */
    for (cpp = argv->argv; *cpp; cpp++) {
	if (!PCF_MATCH_ANY(*cpp) && strchr(*cpp, PCF_MATCH_WILDC_STR[0]) != 0) {
	    argv_free(argv);
	    return (0);
	}
    }

    /*
     * Provide defaults for missing fields.
     */
    while (argv->argc < max_expr)
	argv_add(argv, PCF_MATCH_WILDC_STR, ARGV_END);
    return (argv);
}
