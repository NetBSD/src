/*	$NetBSD: dict.h,v 1.1.1.2.10.1 2013/02/25 00:27:31 tls Exp $	*/

#ifndef _DICT_H_INCLUDED_
#define _DICT_H_INCLUDED_

/*++
/* NAME
/*	dict 3h
/* SUMMARY
/*	dictionary manager
/* SYNOPSIS
/*	#include <dict.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <fcntl.h>

 /*
  * Utility library.
  */
#include <vstream.h>
#include <argv.h>
#include <vstring.h>

 /*
  * Provenance information.
  */
typedef struct DICT_OWNER {
    int     status;			/* see below */
    uid_t   uid;			/* use only if status == UNTRUSTED */
} DICT_OWNER;

#define DICT_OWNER_UNKNOWN	(-1)	/* ex: unauthenticated tcp, proxy */
#define DICT_OWNER_TRUSTED	(!1)	/* ex: root-owned config file */
#define DICT_OWNER_UNTRUSTED	(!0)	/* ex: non-root config file */

 /*
  * Generic dictionary interface - in reality, a dictionary extends this
  * structure with private members to maintain internal state.
  */
typedef struct DICT {
    char   *type;			/* for diagnostics */
    char   *name;			/* for diagnostics */
    int     flags;			/* see below */
    const char *(*lookup) (struct DICT *, const char *);
    int     (*update) (struct DICT *, const char *, const char *);
    int     (*delete) (struct DICT *, const char *);
    int     (*sequence) (struct DICT *, int, const char **, const char **);
    void    (*close) (struct DICT *);
    int     lock_fd;			/* for dict_update() lock */
    int     stat_fd;			/* change detection */
    time_t  mtime;			/* mod time at open */
    VSTRING *fold_buf;			/* key folding buffer */
    DICT_OWNER owner;			/* provenance */
    int     error;			/* last operation only */
} DICT;

extern DICT *dict_alloc(const char *, const char *, ssize_t);
extern void dict_free(DICT *);

extern DICT *dict_debug(DICT *);

#define DICT_DEBUG(d) ((d)->flags & DICT_FLAG_DEBUG ? dict_debug(d) : (d))

#define DICT_FLAG_NONE		(0)
#define DICT_FLAG_DUP_WARN	(1<<0)	/* if file, warn about dups */
#define DICT_FLAG_DUP_IGNORE	(1<<1)	/* if file, ignore dups */
#define DICT_FLAG_TRY0NULL	(1<<2)	/* do not append 0 to key/value */
#define DICT_FLAG_TRY1NULL	(1<<3)	/* append 0 to key/value */
#define DICT_FLAG_FIXED		(1<<4)	/* fixed key map */
#define DICT_FLAG_PATTERN	(1<<5)	/* keys are patterns */
#define DICT_FLAG_LOCK		(1<<6)	/* lock before access */
#define DICT_FLAG_DUP_REPLACE	(1<<7)	/* if file, replace dups */
#define DICT_FLAG_SYNC_UPDATE	(1<<8)	/* if file, sync updates */
#define DICT_FLAG_DEBUG		(1<<9)	/* log access */
/*#define DICT_FLAG_FOLD_KEY	(1<<10)	/* lowercase the lookup key */
#define DICT_FLAG_NO_REGSUB	(1<<11)	/* disallow regexp substitution */
#define DICT_FLAG_NO_PROXY	(1<<12)	/* disallow proxy mapping */
#define DICT_FLAG_NO_UNAUTH	(1<<13)	/* disallow unauthenticated data */
#define DICT_FLAG_FOLD_FIX	(1<<14)	/* case-fold key with fixed-case map */
#define DICT_FLAG_FOLD_MUL	(1<<15)	/* case-fold key with multi-case map */
#define DICT_FLAG_FOLD_ANY	(DICT_FLAG_FOLD_FIX | DICT_FLAG_FOLD_MUL)
#define DICT_FLAG_OPEN_LOCK	(1<<16)	/* open file with exclusive lock */

 /* IMPORTANT: Update the dict_mask[] table when the above changes */

 /*
  * The subsets of flags that control how a map is used. These are relevant
  * mainly for proxymap support. Note: some categories overlap.
  * 
  * DICT_FLAG_IMPL_MASK - flags that are set by the map implementation itself.
  * 
  * DICT_FLAG_PARANOID - requestor flags that forbid the use of insecure map
  * types for security-sensitive operations. These flags are checked by the
  * map implementation itself upon open, lookup etc. requests.
  * 
  * DICT_FLAG_RQST_MASK - all requestor flags, including paranoid flags, that
  * the requestor may change between open, lookup etc. requests. These
  * specify requestor properties, not map properties.
  * 
  * DICT_FLAG_INST_MASK - none of the above flags. The requestor may not change
  * these flags between open, lookup, etc. requests (although a map may make
  * changes to its copy of some of these flags). The proxymap server opens
  * only one map instance for all client requests with the same values of
  * these flags, and the proxymap client uses its own saved copy of these
  * flags.
  */
#define DICT_FLAG_PARANOID \
	(DICT_FLAG_NO_REGSUB | DICT_FLAG_NO_PROXY | DICT_FLAG_NO_UNAUTH)
#define DICT_FLAG_IMPL_MASK	(DICT_FLAG_FIXED | DICT_FLAG_PATTERN)
#define DICT_FLAG_RQST_MASK	(DICT_FLAG_FOLD_ANY | DICT_FLAG_LOCK | \
				DICT_FLAG_DUP_REPLACE | DICT_FLAG_DUP_WARN | \
				DICT_FLAG_DUP_IGNORE | DICT_FLAG_SYNC_UPDATE | \
				DICT_FLAG_PARANOID)
#define DICT_FLAG_INST_MASK	~(DICT_FLAG_IMPL_MASK | DICT_FLAG_RQST_MASK)

 /*
  * dict->error values. Errors must be negative; smtpd_check depends on this.
  */
#define DICT_ERR_NONE	0		/* no error */
#define DICT_ERR_RETRY	(-1)		/* soft error */
#define DICT_ERR_CONFIG	(-2)		/* configuration error */

 /*
  * FAIL/ERROR are suggested result values, not meant for use in comparisons.
  */
#define DICT_STAT_FAIL		1	/* any value > 0: notfound, conflict */
#define DICT_STAT_SUCCESS	0	/* request satisfied */
#define DICT_STAT_ERROR		(-1)	/* any value < 0: database error */

#define DICT_ERR_VAL_RETURN(dict, err, val) do { \
	(dict)->error = (err); \
	return (val); \
    } while (0)

 /*
  * Sequence function types.
  */
#define DICT_SEQ_FUN_FIRST     0	/* set cursor to first record */
#define DICT_SEQ_FUN_NEXT      1	/* set cursor to next record */

 /*
  * Interface for dictionary types.
  */
extern ARGV *dict_mapnames(void);

 /*
  * High-level interface, with logical dictionary names.
  */
extern void dict_register(const char *, DICT *);
extern DICT *dict_handle(const char *);
extern void dict_unregister(const char *);
extern int dict_update(const char *, const char *, const char *);
extern const char *dict_lookup(const char *, const char *);
extern int dict_delete(const char *, const char *);
extern int dict_sequence(const char *, const int, const char **, const char **);
extern int dict_load_file_xt(const char *, const char *);
extern void dict_load_fp(const char *, VSTREAM *);
extern const char *dict_eval(const char *, const char *, int);
extern int dict_error(const char *);

 /*
  * Low-level interface, with physical dictionary handles.
  */
extern DICT *dict_open(const char *, int, int);
extern DICT *dict_open3(const char *, const char *, int, int);
extern void dict_open_register(const char *, DICT *(*) (const char *, int, int));

#define dict_get(dp, key)	((const char *) (dp)->lookup((dp), (key)))
#define dict_put(dp, key, val)	(dp)->update((dp), (key), (val))
#define dict_del(dp, key)	(dp)->delete((dp), (key))
#define dict_seq(dp, f, key, val) (dp)->sequence((dp), (f), (key), (val))
#define dict_close(dp)		(dp)->close(dp)
typedef void (*DICT_WALK_ACTION) (const char *, DICT *, char *);
extern void dict_walk(DICT_WALK_ACTION, char *);
extern int dict_changed(void);
extern const char *dict_changed_name(void);
extern const char *dict_flags_str(int);

 /*
  * Driver for interactive or scripted tests.
  */
void    dict_test(int, char **);

 /*
  * Behind-the-scenes support to continue execution with reduced
  * functionality.
  */
extern int dict_allow_surrogate;
extern DICT *dict_surrogate(const char *, const char *, int, int, const char *,...);

 /*
  * This name is reserved for matchlist error handling.
  */
#define DICT_TYPE_NOFILE	"non-existent"

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
