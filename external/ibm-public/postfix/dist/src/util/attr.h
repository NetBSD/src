/*	$NetBSD: attr.h,v 1.4.2.1 2023/12/25 12:43:36 martin Exp $	*/

#ifndef _ATTR_H_INCLUDED_
#define _ATTR_H_INCLUDED_

/*++
/* NAME
/*	attr 3h
/* SUMMARY
/*	attribute list manipulations
/* SYNOPSIS
/*	#include "attr.h"
 DESCRIPTION
 .nf

 /*
  * System library.
  */
#include <stdarg.h>

 /*
  * Utility library.
  */
#include <vstream.h>
#include <vstring.h>
#include <htable.h>
#include <nvtable.h>
#include <check_arg.h>

 /*
  * Delegation for better data abstraction.
  */
typedef int (*ATTR_SCAN_COMMON_FN) (VSTREAM *, int,...);
typedef int (*ATTR_SCAN_CUSTOM_FN) (ATTR_SCAN_COMMON_FN, VSTREAM *, int, void *);
typedef int (*ATTR_PRINT_COMMON_FN) (VSTREAM *, int,...);
typedef int (*ATTR_PRINT_CUSTOM_FN) (ATTR_PRINT_COMMON_FN, VSTREAM *, int, const void *);

 /*
  * Attribute types. See attr_scan(3) for documentation.
  */
#define ATTR_TYPE_END		0	/* end of data */
#define ATTR_TYPE_INT		1	/* Unsigned integer */
#define ATTR_TYPE_NUM		ATTR_TYPE_INT
#define ATTR_TYPE_STR		2	/* Character string */
#define ATTR_TYPE_HASH		3	/* Hash table */
#define ATTR_TYPE_NV		3	/* Name-value table */
#define ATTR_TYPE_LONG		4	/* Unsigned long */
#define ATTR_TYPE_DATA		5	/* Binary data */
#define ATTR_TYPE_FUNC		6	/* Function pointer */
#define ATTR_TYPE_STREQ		7	/* Requires (name, value) match */

 /*
  * Optional sender-specified grouping for hash or nameval tables.
  */
#define ATTR_TYPE_OPEN		'{'
#define ATTR_TYPE_CLOSE		'}'
#define ATTR_NAME_OPEN		"{"
#define ATTR_NAME_CLOSE		"}"

#define ATTR_HASH_LIMIT		1024	/* Size of hash table */

 /*
  * Typechecking support for variadic function arguments. See check_arg(3h)
  * for documentation.
  */
#define SEND_ATTR_INT(name, val)	ATTR_TYPE_INT, CHECK_CPTR(ATTR, char, (name)), CHECK_VAL(ATTR, int, (val))
#define SEND_ATTR_UINT(name, val)	ATTR_TYPE_INT, CHECK_CPTR(ATTR, char, (name)), CHECK_VAL(ATTR, unsigned, (val))
#define SEND_ATTR_STR(name, val)	ATTR_TYPE_STR, CHECK_CPTR(ATTR, char, (name)), CHECK_CPTR(ATTR, char, (val))
#define SEND_ATTR_HASH(val)		ATTR_TYPE_HASH, CHECK_CPTR(ATTR, HTABLE, (val))
#define SEND_ATTR_NV(val)		ATTR_TYPE_NV, CHECK_CPTR(ATTR, NVTABLE, (val))
#define SEND_ATTR_LONG(name, val)	ATTR_TYPE_LONG, CHECK_CPTR(ATTR, char, (name)), CHECK_VAL(ATTR, long, (val))
#define SEND_ATTR_DATA(name, len, val)	ATTR_TYPE_DATA, CHECK_CPTR(ATTR, char, (name)), CHECK_VAL(ATTR, ssize_t, (len)), CHECK_CPTR(ATTR, void, (val))
#define SEND_ATTR_FUNC(func, val)	ATTR_TYPE_FUNC, CHECK_VAL(ATTR, ATTR_PRINT_CUSTOM_FN, (func)), CHECK_CPTR(ATTR, void, (val))

#define RECV_ATTR_INT(name, val)	ATTR_TYPE_INT, CHECK_CPTR(ATTR, char, (name)), CHECK_PTR(ATTR, int, (val))
#define RECV_ATTR_UINT(name, val)	ATTR_TYPE_INT, CHECK_CPTR(ATTR, char, (name)), CHECK_PTR(ATTR, unsigned, (val))
#define RECV_ATTR_STR(name, val)	ATTR_TYPE_STR, CHECK_CPTR(ATTR, char, (name)), CHECK_PTR(ATTR, VSTRING, (val))
#define RECV_ATTR_STREQ(name, val)	ATTR_TYPE_STREQ, CHECK_CPTR(ATTR, char, (name)), CHECK_CPTR(ATTR, char, (val))
#define RECV_ATTR_HASH(val)		ATTR_TYPE_HASH, CHECK_PTR(ATTR, HTABLE, (val))
#define RECV_ATTR_NV(val)		ATTR_TYPE_NV, CHECK_PTR(ATTR, NVTABLE, (val))
#define RECV_ATTR_LONG(name, val)	ATTR_TYPE_LONG, CHECK_CPTR(ATTR, char, (name)), CHECK_PTR(ATTR, long, (val))
#define RECV_ATTR_DATA(name, val)	ATTR_TYPE_DATA, CHECK_CPTR(ATTR, char, (name)), CHECK_PTR(ATTR, VSTRING, (val))
#define RECV_ATTR_FUNC(func, val)	ATTR_TYPE_FUNC, CHECK_VAL(ATTR, ATTR_SCAN_CUSTOM_FN, (func)), CHECK_PTR(ATTR, void, (val))

CHECK_VAL_HELPER_DCL(ATTR, ssize_t);
CHECK_VAL_HELPER_DCL(ATTR, long);
CHECK_VAL_HELPER_DCL(ATTR, int);
CHECK_VAL_HELPER_DCL(ATTR, unsigned);
CHECK_PTR_HELPER_DCL(ATTR, void);
CHECK_PTR_HELPER_DCL(ATTR, long);
CHECK_PTR_HELPER_DCL(ATTR, int);
CHECK_PTR_HELPER_DCL(ATTR, unsigned);
CHECK_PTR_HELPER_DCL(ATTR, VSTRING);
CHECK_PTR_HELPER_DCL(ATTR, NVTABLE);
CHECK_PTR_HELPER_DCL(ATTR, HTABLE);
CHECK_CPTR_HELPER_DCL(ATTR, void);
CHECK_CPTR_HELPER_DCL(ATTR, char);
CHECK_CPTR_HELPER_DCL(ATTR, NVTABLE);
CHECK_CPTR_HELPER_DCL(ATTR, HTABLE);
CHECK_VAL_HELPER_DCL(ATTR, ATTR_PRINT_CUSTOM_FN);
CHECK_VAL_HELPER_DCL(ATTR, ATTR_SCAN_CUSTOM_FN);

 /*
  * Flags that control processing. See attr_scan(3) for documentation.
  */
#define ATTR_FLAG_NONE		0
#define ATTR_FLAG_MISSING	(1<<0)	/* Flag missing attribute */
#define ATTR_FLAG_EXTRA		(1<<1)	/* Flag spurious attribute */
#define ATTR_FLAG_MORE		(1<<2)	/* Don't skip or terminate */
#define ATTR_FLAG_PRINTABLE	(1<<3)	/* Sanitize received strings */

#define ATTR_FLAG_STRICT	(ATTR_FLAG_MISSING | ATTR_FLAG_EXTRA)
#define ATTR_FLAG_ALL		(017)

 /*
  * Default to null-terminated, as opposed to base64-encoded.
  */
#define attr_print	attr_print0
#define attr_vprint	attr_vprint0
#define attr_scan	attr_scan0
#define attr_vscan	attr_vscan0
#define attr_scan_more	attr_scan_more0

 /*
  * attr_print64.c.
  */
extern int attr_print64(VSTREAM *, int,...);
extern int attr_vprint64(VSTREAM *, int, va_list);

 /*
  * attr_scan64.c.
  */
extern int WARN_UNUSED_RESULT attr_scan64(VSTREAM *, int,...);
extern int WARN_UNUSED_RESULT attr_vscan64(VSTREAM *, int, va_list);
extern int WARN_UNUSED_RESULT attr_scan_more64(VSTREAM *);

 /*
  * attr_print0.c.
  */
extern int attr_print0(VSTREAM *, int,...);
extern int attr_vprint0(VSTREAM *, int, va_list);

 /*
  * attr_scan0.c.
  */
extern int WARN_UNUSED_RESULT attr_scan0(VSTREAM *, int,...);
extern int WARN_UNUSED_RESULT attr_vscan0(VSTREAM *, int, va_list);
extern int WARN_UNUSED_RESULT attr_scan_more0(VSTREAM *);

 /*
  * attr_scan_plain.c.
  */
extern int attr_print_plain(VSTREAM *, int,...);
extern int attr_vprint_plain(VSTREAM *, int, va_list);
extern int attr_scan_more_plain(VSTREAM *);

 /*
  * attr_print_plain.c.
  */
extern int WARN_UNUSED_RESULT attr_scan_plain(VSTREAM *, int,...);
extern int WARN_UNUSED_RESULT attr_vscan_plain(VSTREAM *, int, va_list);

 /*
  * Attribute names for testing the compatibility of the read and write
  * routines.
  */
#ifdef TEST
#define ATTR_NAME_INT		"number"
#define ATTR_NAME_STR		"string"
#define ATTR_NAME_LONG		"long_number"
#define ATTR_NAME_DATA		"data"
#endif

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

#endif
