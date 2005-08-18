/*	$NetBSD: tls_temp.c,v 1.1.1.1 2005/08/18 21:11:10 rpaulo Exp $	*/

/*++
/* NAME
/*	tls_temp 3
/* SUMMARY
/*	code that is to be replaced
/* SYNOPSIS
/*	#define TLS_INTERNAL
/*	#include <tls.h>
/* DESCRIPTION
/*	As the summary says.
/* LICENSE
/* .ad
/* .fi
/*	This software is free. You can do with it whatever you want.
/*	The original author kindly requests that you acknowledge
/*	the use of his software.
/* AUTHOR(S)
/*	Originally written by:
/*	Lutz Jaenicke
/*	BTU Cottbus
/*	Allgemeine Elektrotechnik
/*	Universitaetsplatz 3-4
/*	D-03044 Cottbus, Germany
/*
/*	Updated by:
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>

#ifdef USE_TLS

/* TLS library. */

#define TLS_INTERNAL
#include <tls.h>

/* Application-specific. */

const tls_info_t tls_info_zero = {
    0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0
};

#endif
