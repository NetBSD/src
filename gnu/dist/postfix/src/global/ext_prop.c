/*++
/* NAME
/*	exp_prop 3
/* SUMMARY
/*	address extension propagation control
/* SYNOPSIS
/*	#include <exp_prop.h>
/*
/*	int	ext_prop_mask(pattern)
/*	const char *pattern;
/* DESCRIPTION
/*	This module controld address extension propagation.
/*
/*	ext_prop_mask() takes a comma-separated list of names and
/*	computes the corresponding mask. The following names are
/*	recognized in \fBpattern\fR, with the corresponding bit mask
/*	given in parentheses:
/* .IP "canonical (EXP_PROP_CANONICAL)"
/*	Propagate unmatched address extensions to the right-hand side
/*	of canonical table entries (not: regular expressions).
/* .IP "virtual (EXP_PROP_VIRTUAL)
/*	Propagate unmatched address extensions to the right-hand side
/*	of virtual table entries (not: regular expressions).
/* .IP "alias (EXP_PROP_ALIAS)
/*	Propagate unmatched address extensions to the right-hand side
/*	of alias database entries.
/* .IP "forward (EXP_PROP_FORWARD)"
/*	Propagate unmatched address extensions to the right-hand side
/*	of .forward file entries.
/* .IP "include (EXP_PROP_INCLUDE)"
/*	Propagate unmatched address extensions to the right-hand side
/*	of :include: file entries.
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

int     ext_prop_mask(const char *pattern)
{
    static NAME_MASK table[] = {
	"canonical", EXT_PROP_CANONICAL,
	"virtual", EXT_PROP_VIRTUAL,
	"alias", EXT_PROP_ALIAS,
	"forward", EXT_PROP_FORWARD,
	"include", EXT_PROP_INCLUDE,
	0,
    };

    return (name_mask(VAR_PROP_EXTENSION, table, pattern));
}
