/*	$NetBSD: mkmap_cdb.c,v 1.1.1.1 2009/06/23 10:08:47 tron Exp $	*/

/*++
/* NAME
/*	mkmap_cdb 3
/* SUMMARY
/*	create or open database, CDB style
/* SYNOPSIS
/*	#include <mkmap.h>
/*
/*	MKMAP	*mkmap_cdb_open(path)
/*	const char *path;
/*
/* DESCRIPTION
/*	This module implements support for creating DJB's CDB "constant
/*	databases".
/*
/*	mkmap_cdb_open() take a file name, append the ".cdb.tmp" suffix,
/*	create the named DB database.  On close, this file renamed to
/*	file name with ".cdb" suffix appended (without ".tmp" part).
/*	This routine is a CDB-specific helper for the more
/*	general mkmap_open() interface.
/*
/*	All errors are fatal.
/* SEE ALSO
/*	dict_cdb(3), CDB dictionary interface.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Written by Michael Tokarev <mjt@tls.msk.ru> based on mkmap_db by
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>

#ifdef HAS_CDB

/* Utility library. */

#include <mymalloc.h>
#include <dict.h>

/* Application-specific. */

#include "mkmap.h"
#include <dict_cdb.h>

/* This is a dummy module, since CDB has all the functionality
 * built-in, as cdb creation requires one global lock anyway. */

MKMAP *mkmap_cdb_open(const char *unused_path)
{
    MKMAP  *mkmap = (MKMAP *) mymalloc(sizeof(*mkmap));
    mkmap->open = dict_cdb_open;
    mkmap->after_open = 0;
    mkmap->after_close = 0;
    return (mkmap);
}

#endif /* HAS_CDB */
