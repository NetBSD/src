/*	$NetBSD: match_list.h,v 1.1.1.1.10.1 2013/01/23 00:05:16 yamt Exp $	*/

#ifndef _MATCH_LIST_H_INCLUDED_
#define _MATCH_LIST_H_INCLUDED_

/*++
/* NAME
/*	match_list 3h
/* SUMMARY
/*	generic list-based pattern matching
/* SYNOPSIS
/*	#include <match_list.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <argv.h>

 /*
  * External interface.
  */
typedef struct MATCH_LIST MATCH_LIST;

typedef int (*MATCH_LIST_FN) (MATCH_LIST *, const char *, const char *);

struct MATCH_LIST {
    int     flags;			/* processing options */
    ARGV   *patterns;			/* one pattern each */
    int     match_count;		/* match function/argument count */
    MATCH_LIST_FN *match_func;		/* match functions */
    const char **match_args;		/* match arguments */
    int     error;			/* last operation */
};

#define MATCH_FLAG_NONE		0
#define MATCH_FLAG_PARENT	(1<<0)
#define MATCH_FLAG_RETURN	(1<<1)
#define MATCH_FLAG_ALL		(MATCH_FLAG_PARENT | MATCH_FLAG_RETURN)

extern MATCH_LIST *match_list_init(int, const char *, int,...);
extern int match_list_match(MATCH_LIST *,...);
extern void match_list_free(MATCH_LIST *);

 /*
  * The following functions are not part of the public interface. These
  * functions may be called only through match_list_match().
  */
extern int match_string(MATCH_LIST *, const char *, const char *);
extern int match_hostname(MATCH_LIST *, const char *, const char *);
extern int match_hostaddr(MATCH_LIST *, const char *, const char *);

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

