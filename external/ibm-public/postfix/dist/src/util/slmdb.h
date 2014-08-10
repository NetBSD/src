/*	$NetBSD: slmdb.h,v 1.1.1.1.2.2 2014/08/10 07:12:50 tls Exp $	*/

#ifndef _SLMDB_H_INCLUDED_
#define _SLMDB_H_INCLUDED_

/*++
/* NAME
/*	slmdb 3h
/* SUMMARY
/*	LMDB API wrapper
/* SYNOPSIS
/*	#include <slmdb.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <setjmp.h>

#ifdef PATH_LMDB_H
#include PATH_LMDB_H
#else
#include <lmdb.h>
#endif

 /*
  * External interface.
  */
#ifdef NO_SIGSETJMP
#define SLMDB_JMP_BUF jmp_buf
#else
#define SLMDB_JMP_BUF sigjmp_buf
#endif

 /*
  * All data structure members are private.
  */
typedef struct {
    size_t  curr_limit;			/* database soft size limit */
    int     size_incr;			/* database expansion factor */
    size_t  hard_limit;			/* database hard size limit */
    int     open_flags;			/* open() flags */
    int     lmdb_flags;			/* LMDB-specific flags */
    int     slmdb_flags;		/* bulk-mode flag */
    MDB_env *env;			/* database environment */
    MDB_dbi dbi;			/* database instance */
    MDB_txn *txn;			/* bulk transaction */
    int     db_fd;			/* database file handle */
    MDB_cursor *cursor;			/* iterator */
    MDB_val saved_key;			/* saved cursor key buffer */
    size_t  saved_key_size;		/* saved cursor key buffer size */
    void    (*longjmp_fn) (void *, int);/* exception handling */
    void    (*notify_fn) (void *, int,...);	/* workaround notification */
    void    (*assert_fn) (void *, const char *);	/* assert notification */
    void   *cb_context;			/* call-back context */
    int     api_retry_count;		/* slmdb(3) API call retry count */
    int     bulk_retry_count;		/* bulk_mode retry count */
    int     api_retry_limit;		/* slmdb(3) API call retry limit */
    int     bulk_retry_limit;		/* bulk_mode retry limit */
} SLMDB;

#define SLMDB_FLAG_BULK		(1 << 0)

extern int slmdb_init(SLMDB *, size_t, int, size_t);
extern int slmdb_open(SLMDB *, const char *, int, int, int);
extern int slmdb_get(SLMDB *, MDB_val *, MDB_val *);
extern int slmdb_put(SLMDB *, MDB_val *, MDB_val *, int);
extern int slmdb_del(SLMDB *, MDB_val *);
extern int slmdb_cursor_get(SLMDB *, MDB_val *, MDB_val *, MDB_cursor_op);
extern int slmdb_control(SLMDB *, int,...);
extern int slmdb_close(SLMDB *);

#define slmdb_fd(slmdb)			((slmdb)->db_fd)
#define slmdb_curr_limit(slmdb)		((slmdb)->curr_limit)

#define SLMDB_CTL_END		0
#define SLMDB_CTL_LONGJMP_FN	1	/* exception handling */
#define SLMDB_CTL_NOTIFY_FN	2	/* debug logging function */
#define SLMDB_CTL_CB_CONTEXT	3	/* call-back context */
#define SLMDB_CTL_HARD_LIMIT	4	/* hard database size limit */
#define SLMDB_CTL_API_RETRY_LIMIT	5	/* per slmdb(3) API call */
#define SLMDB_CTL_BULK_RETRY_LIMIT	6	/* per bulk update */
#define SLMDB_CTL_ASSERT_FN	7	/* report assertion failure */

typedef void (*SLMDB_NOTIFY_FN) (void *, int,...);
typedef void (*SLMDB_LONGJMP_FN) (void *, int);
typedef void (*SLMDB_ASSERT_FN) (void *, const char *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Howard Chu
/*	Symas Corporation
/*--*/

#endif
