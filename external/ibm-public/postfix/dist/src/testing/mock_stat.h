/*	$NetBSD: mock_stat.h,v 1.1.1.1 2026/05/09 18:39:22 christos Exp $	*/

#ifndef _MOCK_STAT_H_INCLUDED_
#define _MOCK_STAT_H_INCLUDED_

/*++
/* NAME
/*	mock_stat 3h
/* SUMMARY
/*	mock stat
/* SYNOPSIS
/*	#include <mock_stat.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <sys/stat.h>
#include <unistd.h>

 /*
  * External interface.
  */
typedef struct MOCK_STAT_REQ {
    const char *want_path;
    struct stat out_st;
    int     out_errno;
} MOCK_STAT_REQ;

extern void setup_mock_stat(const MOCK_STAT_REQ *);
extern void teardown_mock_stat(void);

/* LICENSE
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
