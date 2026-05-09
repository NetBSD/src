/*	$NetBSD: mock_open_as.h,v 1.2 2026/05/09 18:49:21 christos Exp $	*/

#ifndef _MOCK_OPEN_AS_H_INCLUDED_
#define _MOCK_OPEN_AS_H_INCLUDED_

/*++
/* NAME
/*	mock_open_as 3h
/* SUMMARY
/*	fake open_as
/* SYNOPSIS
/*	#include <mock_open_as.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <unistd.h>

 /*
  * Utility library.
  */
#include <open_as.h>
#include <vstream.h>

 /*
  * External interface.
  */
typedef struct MOCK_OPEN_AS_REQ {
    const char *want_path;
    uid_t   want_uid;
    gid_t   want_gid;
    const char *out_data;
    int     out_errno;
} MOCK_OPEN_AS_REQ;

extern void setup_mock_vstream_fopen_as(const MOCK_OPEN_AS_REQ *);

/* LICENSE
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
