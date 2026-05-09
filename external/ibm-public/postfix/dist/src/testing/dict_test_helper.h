/*	$NetBSD: dict_test_helper.h,v 1.2 2026/05/09 18:49:21 christos Exp $	*/

#ifndef _DICT_TEST_HELPER_H_INCLUDED_
#define _DICT_TEST_HELPER_H_INCLUDED_

/*++
/* NAME
/*	dict_test_helper 3h
/* SUMMARY
/*	dictionary test helpers
/* SYNOPSIS
/*	#include <dict_test_helper.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <argv.h>
#include <dict.h>
#include <vstring.h>

 /*
  * External interface.
  */
extern DICT *dict_open_and_capture_msg(const char *, int, int, VSTRING *);
extern char *dict_compose_spec(const char *, const char **, int, int, VSTRING *, ARGV *);
extern const char *dict_get_and_capture_msg(DICT *, const char *, VSTRING *);

struct dict_get_verify_data {
    const char *key;
    const char *want_value;
    int     want_error;
    const char *want_msg;
};

extern int dict_get_and_verify(DICT *, const char *, const char *, int, const char *);
extern int dict_get_and_verify_bulk(DICT *, const struct dict_get_verify_data *);

/* LICENSE
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
