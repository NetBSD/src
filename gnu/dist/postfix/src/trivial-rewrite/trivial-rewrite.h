/*++
/* NAME
/*	trivial-rewrite 3h
/* SUMMARY
/*	mail address rewriter and resolver
/* SYNOPSIS
/*	#include "trivial-rewrite.h"
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>
#include <vstream.h>

 /*
  * Global library.
  */
#include <tok822.h>

 /*
  * rewrite.c
  */
extern void rewrite_init(void);
extern int rewrite_proto(VSTREAM *);
extern void rewrite_addr(char *, char *, VSTRING *);
extern void rewrite_tree(char *, TOK822 *);

 /*
  * resolve.c
  */
extern void resolve_init(void);
extern int resolve_proto(VSTREAM *);
extern void resolve_addr(char *, VSTRING *, VSTRING *, VSTRING *, int *);

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

