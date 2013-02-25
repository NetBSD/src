/*	$NetBSD: mymalloc.c,v 1.1.1.2.10.1 2013/02/25 00:27:32 tls Exp $	*/

/*++
/* NAME
/*	mymalloc 3
/* SUMMARY
/*	memory management wrappers
/* SYNOPSIS
/*	#include <mymalloc.h>
/*
/*	char	*mymalloc(len)
/*	ssize_t	len;
/*
/*	char	*myrealloc(ptr, len)
/*	char	*ptr;
/*	ssize_t	len;
/*
/*	void	myfree(ptr)
/*	char	*ptr;
/*
/*	char	*mystrdup(str)
/*	const char *str;
/*
/*	char	*mystrndup(str, len)
/*	const char *str;
/*	ssize_t	len;
/*
/*	char	*mymemdup(ptr, len)
/*	const char *ptr;
/*	ssize_t	len;
/* DESCRIPTION
/*	This module performs low-level memory management with error
/*	handling. A call of these functions either succeeds or it does
/*	not return at all.
/*
/*	To save memory, zero-length strings are shared and read-only.
/*	The caller must not attempt to modify the null terminator.
/*	This code is enabled unless NO_SHARED_EMPTY_STRINGS is
/*	defined at compile time (for example, you have an sscanf()
/*	routine that pushes characters back into its input).
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
/*	with length \fIlen\fR. The result is NOT null-terminated.
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
    ssize_t length;			/* user requested length */
    union {
	ALIGN_TYPE align;
	char    payload[1];		/* actually a bunch of bytes */
    }       u;
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

 /*
  * Optimization for short strings. We share one copy with multiple callers.
  * This differs from normal heap memory in two ways, because the memory is
  * shared:
  * 
  * -  It must be read-only to avoid horrible bugs. This is OK because there is
  * no legitimate reason to modify the null terminator.
  * 
  * - myfree() cannot overwrite the memory with a filler pattern like it can do
  * with heap memory. Therefore, some dangling pointer bugs will be masked.
  */
#ifndef NO_SHARED_EMPTY_STRINGS
static const char empty_string[] = "";

#endif

/* mymalloc - allocate memory or bust */

char   *mymalloc(ssize_t len)
{
    char   *ptr;
    MBLOCK *real_ptr;

    /*
     * Note: for safety reasons the request length is a signed type. This
     * allows us to catch integer overflow problems that weren't already
     * caught up-stream.
     */
    if (len < 1)
	msg_panic("mymalloc: requested length %ld", (long) len);
#ifdef MYMALLOC_FUZZ
    len += MYMALLOC_FUZZ;
#endif
    if ((real_ptr = (MBLOCK *) malloc(SPACE_FOR(len))) == 0)
	msg_fatal("mymalloc: insufficient memory for %ld bytes: %m",
		  (long) len);
    CHECK_OUT_PTR(ptr, real_ptr, len);
    memset(ptr, FILLER, len);
    return (ptr);
}

/* myrealloc - reallocate memory or bust */

char   *myrealloc(char *ptr, ssize_t len)
{
    MBLOCK *real_ptr;
    ssize_t old_len;

#ifndef NO_SHARED_EMPTY_STRINGS
    if (ptr == empty_string)
	return (mymalloc(len));
#endif

    /*
     * Note: for safety reasons the request length is a signed type. This
     * allows us to catch integer overflow problems that weren't already
     * caught up-stream.
     */
    if (len < 1)
	msg_panic("myrealloc: requested length %ld", (long) len);
#ifdef MYMALLOC_FUZZ
    len += MYMALLOC_FUZZ;
#endif
    CHECK_IN_PTR(ptr, real_ptr, old_len, "myrealloc");
    if ((real_ptr = (MBLOCK *) realloc((char *) real_ptr, SPACE_FOR(len))) == 0)
	msg_fatal("myrealloc: insufficient memory for %ld bytes: %m",
		  (long) len);
    CHECK_OUT_PTR(ptr, real_ptr, len);
    if (len > old_len)
	memset(ptr + old_len, FILLER, len - old_len);
    return (ptr);
}

/* myfree - release memory */

void    myfree(char *ptr)
{
    MBLOCK *real_ptr;
    ssize_t len;

#ifndef NO_SHARED_EMPTY_STRINGS
    if (ptr != empty_string) {
#endif
	CHECK_IN_PTR(ptr, real_ptr, len, "myfree");
	memset((char *) real_ptr, FILLER, SPACE_FOR(len));
	free((char *) real_ptr);
#ifndef NO_SHARED_EMPTY_STRINGS
    }
#endif
}

/* mystrdup - save string to heap */

char   *mystrdup(const char *str)
{
    if (str == 0)
	msg_panic("mystrdup: null pointer argument");
#ifndef NO_SHARED_EMPTY_STRINGS
    if (*str == 0)
	return ((char *) empty_string);
#endif
    return (strcpy(mymalloc(strlen(str) + 1), str));
}

/* mystrndup - save substring to heap */

char   *mystrndup(const char *str, ssize_t len)
{
    char   *result;
    char   *cp;

    if (str == 0)
	msg_panic("mystrndup: null pointer argument");
    if (len < 0)
	msg_panic("mystrndup: requested length %ld", (long) len);
#ifndef NO_SHARED_EMPTY_STRINGS
    if (*str == 0)
	return ((char *) empty_string);
#endif
    if ((cp = memchr(str, 0, len)) != 0)
	len = cp - str;
    result = memcpy(mymalloc(len + 1), str, len);
    result[len] = 0;
    return (result);
}

/* mymemdup - copy memory */

char   *mymemdup(const char *ptr, ssize_t len)
{
    if (ptr == 0)
	msg_panic("mymemdup: null pointer argument");
    return (memcpy(mymalloc(len), ptr, len));
}
