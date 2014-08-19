/*	$NetBSD: mkmap_lmdb.c,v 1.1.1.1.6.2 2014/08/19 23:59:42 tls Exp $	*/

/*++
/* NAME
/*	mkmap_lmdb 3
/* SUMMARY
/*	create or open database, LMDB style
/* SYNOPSIS
/*	#include <mkmap.h>
/*
/*	MKMAP	*mkmap_lmdb_open(path)
/*	const char *path;
/*
/* DESCRIPTION
/*	This module implements support for creating LMDB databases.
/*
/*	mkmap_lmdb_open() takes a file name, appends the ".lmdb"
/*	suffix, and does whatever initialization is required
/*	before the OpenLDAP LMDB open routine is called.
/*
/*	All errors are fatal.
/* SEE ALSO
/*	dict_lmdb(3), LMDB dictionary interface.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Howard Chu
/*	Symas Corporation
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <dict.h>
#include <dict_lmdb.h>
#include <myflock.h>
#include <warn_stat.h>

#ifdef HAS_LMDB
#ifdef PATH_LMDB_H
#include PATH_LMDB_H
#else
#include <lmdb.h>
#endif

/* Global library. */

#include <mail_conf.h>
#include <mail_params.h>

/* Application-specific. */

#include "mkmap.h"

/* mkmap_lmdb_open */

MKMAP  *mkmap_lmdb_open(const char *path)
{
    MKMAP  *mkmap = (MKMAP *) mymalloc(sizeof(*mkmap));

    /*
     * Fill in the generic members.
     */
    mkmap->open = dict_lmdb_open;
    mkmap->after_open = 0;
    mkmap->after_close = 0;

    /*
     * LMDB uses MVCC so it needs no special lock management here.
     */

    return (mkmap);
}

#endif
