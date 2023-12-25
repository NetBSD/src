/*	$NetBSD: mkmap_fail.c,v 1.2.2.2 2023/12/25 12:43:38 martin Exp $	*/

/*++
/* NAME
/*	mkmap_fail 3
/* SUMMARY
/*	create or open database, fail: style
/* SYNOPSIS
/*	#include <dict_fail.h>
/*
/*	MKMAP	*mkmap_fail_open(path)
/*	const char *path;
/*
/* DESCRIPTION
/*	This module implements support for error testing postmap
/*	and postalias with the fail: table type.
/* SEE ALSO
/*	dict_fail(3), fail dictionary interface.
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
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <mymalloc.h>
#include <dict_fail.h>

 /*
  * Dummy module: the dict_fail module has all the functionality built-in.
  */
MKMAP  *mkmap_fail_open(const char *unused_path)
{
    MKMAP  *mkmap = (MKMAP *) mymalloc(sizeof(*mkmap));

    mkmap->open = dict_fail_open;
    mkmap->after_open = 0;
    mkmap->after_close = 0;
    return (mkmap);
}
