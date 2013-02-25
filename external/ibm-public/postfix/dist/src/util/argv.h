/*	$NetBSD: argv.h,v 1.1.1.1.16.1 2013/02/25 00:27:31 tls Exp $	*/

#ifndef _ARGV_H_INCLUDED_
#define _ARGV_H_INCLUDED_

/*++
/* NAME
/*	argv 3h
/* SUMMARY
/*	string array utilities
/* SYNOPSIS
/*	#include "argv.h"
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
typedef struct ARGV {
    ssize_t len;			/* number of array elements */
    ssize_t argc;			/* array elements in use */
    char  **argv;			/* string array */
} ARGV;

extern ARGV *argv_alloc(ssize_t);
extern void argv_add(ARGV *,...);
extern void argv_addn(ARGV *,...);
extern void argv_terminate(ARGV *);
extern void argv_truncate(ARGV *, ssize_t);
extern void argv_insert_one(ARGV *, ssize_t, const char *);
extern void argv_replace_one(ARGV *, ssize_t, const char *);
extern ARGV *argv_free(ARGV *);

extern ARGV *argv_split(const char *, const char *);
extern ARGV *argv_split_count(const char *, const char *, ssize_t);
extern ARGV *argv_split_append(ARGV *, const char *, const char *);

#define ARGV_FAKE_BEGIN(fake_argv, arg) { \
	ARGV fake_argv; \
	char *__fake_argv_args__[2]; \
	__fake_argv_args__[0] = (char *) (arg); \
	__fake_argv_args__[1] = 0; \
	fake_argv.argv = __fake_argv_args__; \
	fake_argv.argc = fake_argv.len = 1;

#define ARGV_FAKE_END	}

#define ARGV_END	((char *) 0)

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

#endif
