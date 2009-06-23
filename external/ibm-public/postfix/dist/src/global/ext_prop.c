/*	$NetBSD: ext_prop.c,v 1.1.1.1 2009/06/23 10:08:46 tron Exp $	*/

/*++
/* NAME
/*	ext_prop 3
/* SUMMARY
/*	address extension propagation control
/* SYNOPSIS
/*	#include <ext_prop.h>
/*
/*	int	ext_prop_mask(param_name, pattern)
/*	const char *param_name;
/*	const char *pattern;
/* DESCRIPTION
/*	This module controls address extension propagation.
/*
/*	ext_prop_mask() takes a comma-separated list of names and
/*	computes the corresponding mask. The following names are
/*	recognized in \fBpattern\fR, with the corresponding bit mask
/*	given in parentheses:
/* .IP "canonical (EXT_PROP_CANONICAL)"
/*	Propagate unmatched address extensions to the right-hand side
/*	of canonical table entries (not: regular expressions).
/* .IP "virtual (EXT_PROP_VIRTUAL)
/*	Propagate unmatched address extensions to the right-hand side
/*	of virtual table entries (not: regular expressions).
/* .IP "alias (EXT_PROP_ALIAS)
/*	Propagate unmatched address extensions to the right-hand side
/*	of alias database entries.
/* .IP "forward (EXT_PROP_FORWARD)"
/*	Propagate unmatched address extensions to the right-hand side
/*	of .forward file entries.
/* .IP "include (EXT_PROP_INCLUDE)"
/*	Propagate unmatched address extensions to the right-hand side
/*	of :include: file entries.
/* .IP "generic (EXT_PROP_GENERIC)"
/*	Propagate unmatched address extensions to the right-hand side
/*	of smtp_generic_maps entries.
/* DIAGNOSTICS
/*	Panic: inappropriate use.
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

/* Utility library. */

#include <name_mask.h>

/* Global library. */

#include <mail_params.h>
#include <ext_prop.h>

/* ext_prop_mask - compute extension propagation mask */

int     ext_prop_mask(const char *param_name, const char *pattern)
{
    static const NAME_MASK table[] = {
	"canonical", EXT_PROP_CANONICAL,
	"virtual", EXT_PROP_VIRTUAL,
	"alias", EXT_PROP_ALIAS,
	"forward", EXT_PROP_FORWARD,
	"include", EXT_PROP_INCLUDE,
	"generic", EXT_PROP_GENERIC,
	0,
    };

    return (name_mask(param_name, table, pattern));
}
