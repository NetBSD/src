/*	$NetBSD: mkmap_proxy.c,v 1.1.1.1 2009/06/23 10:08:47 tron Exp $	*/

/*++
/* NAME
/*	mkmap_proxy 3
/* SUMMARY
/*	create or proxied database
/* SYNOPSIS
/*	#include <mkmap.h>
/*
/*	MKMAP	*mkmap_proxy_open(path)
/*	const char *path;
/* DESCRIPTION
/*	This module implements support for updating proxy databases.
/*
/*	mkmap_proxy_open() is a proxymap-specific helper for the
/*	more general mkmap_open() routine.
/*
/*	All errors are fatal.
/* SEE ALSO
/*	dict_proxy(3), proxy client interface.
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

#include <mymalloc.h>
#include <dict_proxy.h>

/* Application-specific. */

#include "mkmap.h"

/* mkmap_proxy_open - create or open database */

MKMAP  *mkmap_proxy_open(const char *unused_path)
{
    MKMAP *mkmap = (MKMAP *) mymalloc(sizeof(*mkmap));

    /*
     * Fill in the generic members.
     */
    mkmap->open = dict_proxy_open;
    mkmap->after_open = 0;
    mkmap->after_close = 0;

    return (mkmap);
}
