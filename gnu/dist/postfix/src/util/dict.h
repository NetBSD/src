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

 /*
  * Generic dictionary interface - in reality, a dictionary extends this
  * structure with private members to maintain internal state.
  */
typedef struct DICT {
    char   *type;			/* for diagnostics */
    char   *name;			/* for diagnostics */
    int     flags;			/* see below */
    const char *(*lookup) (struct DICT *, const char *);
    void    (*update) (struct DICT *, const char *, const char *);
    int     (*delete) (struct DICT *, const char *);
    int     (*sequence) (struct DICT *, int, const char **, const char **);
    void    (*close) (struct DICT *);
    int     fd;				/* for dict_update() lock */
    time_t  mtime;			/* mod time at open */
} DICT;

extern DICT *dict_alloc(const char *, const char *, int);
extern void dict_free(DICT *);

extern DICT *dict_debug(DICT *);
#define DICT_DEBUG(d) ((d)->flags & DICT_FLAG_DEBUG ? dict_debug(d) : (d))

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
#define DICT_FLAG_FOLD_KEY	(1<<10)	/* lowercase the lookup key */

extern int dict_unknown_allowed;
extern int dict_errno;

#define DICT_ERR_NONE	0		/* no error */
#define DICT_ERR_RETRY	1		/* soft error */

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
extern void dict_update(const char *, const char *, const char *);
extern const char *dict_lookup(const char *, const char *);
extern int dict_delete(const char *, const char *);
extern int dict_sequence(const char *, const int, const char **, const char **);
extern void dict_load_file(const char *, const char *);
extern void dict_load_fp(const char *, VSTREAM *);
extern const char *dict_eval(const char *, const char *, int);

 /*
  * Low-level interface, with physical dictionary handles.
  */
extern DICT *dict_open(const char *, int, int);
extern DICT *dict_open3(const char *, const char *, int, int);
extern void dict_open_register(const char *, DICT *(*) (const char *, int, int));

#define dict_get(dp, key)	(dp)->lookup((dp), (key))
#define dict_put(dp, key, val)	(dp)->update((dp), (key), (val))
#define dict_del(dp, key)	(dp)->delete((dp), (key))
#define dict_seq(dp, f, key, val) (dp)->sequence((dp), (f), (key), (val))
#define dict_close(dp)		(dp)->close(dp)
typedef void (*DICT_WALK_ACTION) (const char *, DICT *, char *);
extern void dict_walk(DICT_WALK_ACTION, char *);
extern int dict_changed(void);

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
