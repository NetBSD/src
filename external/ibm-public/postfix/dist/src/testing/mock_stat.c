/*	$NetBSD: mock_stat.c,v 1.2 2026/05/09 18:49:21 christos Exp $	*/

/*++
/* NAME
/*	mock_stat 3
/* SUMMARY
/*	test scaffolding
/* SYNOPSIS
/*	#include <mock_stat.h>
/*
/*	typedef struct MOCK_STAT_REQ {
/* .in +4
/*	const char *want_path;
/*	int out_errno;
/*	struct stat st;
/* .in -4
/*	} MOCK_STAT_REQ;
/*
/*	void	setup_mock_stat(const MOCK_STAT_REQ *request)
/*
/*	void	teardown_mock_stat(void)
/* DESCRIPTION
/*	This module provides a mock stat() implementation that returns
/*	static results for expected calls.
/*
/*	setup_mock_stat() adds a mapping from pathname to struct
/*	stat_info pointer. It is an error to enter multiple mappings
/*	for one pathname. If makes a shallow copy ig its input:
/* .IP want_path
/*	The lookup key for file status information.
/* .IP out_errno
/*	If non-zero, mock stat() will return -1 and set the global
/*	errno variable.
/* .IP st
/*	File status information, ignored when out_errno is non-zero.
/* .PP
/*	teardown_mock_stat() destroys all mappings entered with
/*	setup_mock_stat().
/*
/*	The mock stat() function looks up the MOCK_STAT_REQ information
/*	for a pathname and terminates the test if the pathname is not
/*	expected. If the mapping contains a non-zero out_errno value,
/*	it simulates an error, sets the global errno value, and returns
/*	-1.  Otherwise it copies the stat_info.st value to the storage
/*	provided with the st argument.
/* SEE ALSO
/*	stat(2), the real thing
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <errno.h>
#include <sys/stat.h>

 /*
  * Utility library.
  */
#include <htable.h>
#include <msg.h>
#include <wrap_stat.h>

 /*
  * Test library.
  */
#include <mock_stat.h>

 /*
  * Application-specific.
  */
static HTABLE *mock_stat_table;

/* setup_mock_stat - add path-to-status mapping */

void    setup_mock_stat(const MOCK_STAT_REQ *request)
{
    if (mock_stat_table == 0)
	mock_stat_table = htable_create(10);
    htable_enter(mock_stat_table, request->want_path, (void *) request);
}

/* teardown_mock_stat - destroy all path-to-status mappings */

void    teardown_mock_stat(void)
{
    if (mock_stat_table) {
	htable_free(mock_stat_table, (void (*) (void *)) 0);
	mock_stat_table = 0;
    }
}

/* stat - mock stat() implementation */

int stat(const char *path, struct stat *st) {
    MOCK_STAT_REQ *mock_info;

    if ((mock_info = (MOCK_STAT_REQ *) htable_find(mock_stat_table, path)) == 0)
	msg_fatal("mock stat: unexpected pathname: '%s'", path);

    if (mock_info->out_errno != 0) {
	errno = mock_info->out_errno;
	return (-1);
    } else {
	*st = mock_info->out_st;
	return (0);
    }
}
