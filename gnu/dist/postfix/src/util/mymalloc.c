/*++
/* NAME
/*	mymalloc 3
/* SUMMARY
/*	memory management wrappers
/* SYNOPSIS
/*	#include <mymalloc.h>
/*
/*	char	*mymalloc(len)
/*	int	len;
/*
/*	char	*myrealloc(ptr, len)
/*	char	*ptr;
/*	int	len;
/*
/*	void	myfree(ptr)
/*	char	*ptr;
/*
/*	char	*mystrdup(str)
/*	const char *str;
/*
/*	char	*mystrndup(str, len)
/*	const char *str;
/*	int	len;
/*
/*	char	*mymemdup(ptr, len)
/*	const char *ptr;
/*	int	len;
/* DESCRIPTION
/*	This module performs low-level memory management with error
/*	handling. A call of these functions either succeeds or it does
/*	not return at all.
/*
/*	mymalloc() allocates the requested amount of memory. The memory
/*	is not set to zero.
/*
/*	myrealloc() resizes memory obtained from mymalloc() or myrealloc()
/*	to the requested size. The result pointer value may differ from
/*	that given via the \fIptr\fR argument.
/*
/*	myfree() takes memory obtained from mymalloc() or myrealloc()
/*	and makes it available for other use.
/*
/*	mystrdup() returns a dynamic-memory copy of its null-terminated
/*	argument. This routine uses mymalloc().
/*
/*	mystrndup() returns a dynamic-memory copy of at most \fIlen\fR
/*	leading characters of its null-terminated
/*	argument. The result is null-terminated. This routine uses mymalloc().
/*
/*	mymemdup() makes a copy of the memory pointed to by \fIptr\fR
/*	with length \fIlen\fR. The result is null-terminated.
/*	This routine uses mymalloc().
/* SEE ALSO
/*	msg(3) diagnostics interface
/* DIAGNOSTICS
/*	Problems are reported via the msg(3) diagnostics routines:
/*	the requested amount of memory is not available; improper use
/*	is detected; other fatal errors.
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

/* System libraries. */

#include "sys_defs.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

/* Application-specific. */

#include "msg.h"
#include "mymalloc.h"

 /*
  * Structure of an annotated memory block. In order to detect spurious
  * free() calls we prepend a signature to memory given to the application.
  * In order to detect access to free()d blocks, overwrite each block as soon
  * as it is passed to myfree(). With the code below, the user data has
  * integer alignment or better.
  */
typedef struct MBLOCK {
    int     signature;			/* set when block is active */
    int     length;			/* user requested length */
    union {
	ALIGN_TYPE align;
	char    payload[1];		/* actually a bunch of bytes */
    } u;
} MBLOCK;

#define SIGNATURE	0xdead
#define FILLER		0xff

#define CHECK_IN_PTR(ptr, real_ptr, len, fname) { \
    if (ptr == 0) \
	msg_panic("%s: null pointer input", fname); \
    real_ptr = (MBLOCK *) (ptr - offsetof(MBLOCK, u.payload[0])); \
    if (real_ptr->signature != SIGNATURE) \
	msg_panic("%s: corrupt or unallocated memory block", fname); \
    real_ptr->signature = 0; \
    if ((len = real_ptr->length) < 1) \
	msg_panic("%s: corrupt memory block length", fname); \
}

#define CHECK_OUT_PTR(ptr, real_ptr, len) { \
    real_ptr->signature = SIGNATURE; \
    real_ptr->length = len; \
    ptr = real_ptr->u.payload; \
}

#define SPACE_FOR(len)	(offsetof(MBLOCK, u.payload[0]) + len)

/* mymalloc - allocate memory or bust */

char   *mymalloc(int len)
{
    char   *ptr;
    MBLOCK *real_ptr;

    if (len < 1)
	msg_panic("mymalloc: requested length %d", len);
    if ((real_ptr = (MBLOCK *) malloc(SPACE_FOR(len))) == 0)
	msg_fatal("mymalloc: insufficient memory: %m");
    CHECK_OUT_PTR(ptr, real_ptr, len);
    memset(ptr, FILLER, len);
    return (ptr);
}

/* myrealloc - reallocate memory or bust */

char   *myrealloc(char *ptr, int len)
{
    MBLOCK *real_ptr;
    int     old_len;

    if (len < 1)
	msg_panic("myrealloc: requested length %d", len);
    CHECK_IN_PTR(ptr, real_ptr, old_len, "myrealloc");
    if ((real_ptr = (MBLOCK *) realloc((char *) real_ptr, SPACE_FOR(len))) == 0)
	msg_fatal("myrealloc: insufficient memory: %m");
    CHECK_OUT_PTR(ptr, real_ptr, len);
    if (len > old_len)
	memset(ptr + old_len, FILLER, len - old_len);
    return (ptr);
}

/* myfree - release memory */

void    myfree(char *ptr)
{
    MBLOCK *real_ptr;
    int     len;

    CHECK_IN_PTR(ptr, real_ptr, len, "myfree");
    memset((char *) real_ptr, FILLER, SPACE_FOR(len));
    free((char *) real_ptr);
}

/* mystrdup - save string to heap */

char   *mystrdup(const char *str)
{
    if (str == 0)
	msg_panic("mystrdup: null pointer argument");
    return (strcpy(mymalloc(strlen(str) + 1), str));
}

/* mystrndup - save substring to heap */

char   *mystrndup(const char *str, int len)
{
    char   *result;
    char   *cp;

    if (str == 0)
	msg_panic("mystrndup: null pointer argument");
    if ((cp = memchr(str, 0, len)) != 0)
	len = cp - str;
    result = memcpy(mymalloc(len + 1), str, len);
    result[len] = 0;
    return (result);
}

/* mymemdup - copy memory */

char   *mymemdup(const char *ptr, int len)
{
    if (ptr == 0)
	msg_panic("mymemdup: null pointer argument");
    return (memcpy(mymalloc(len), ptr, len));
}
