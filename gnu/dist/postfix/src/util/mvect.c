/*++
/* NAME
/*	mvect 3
/* SUMMARY
/*	memory vector management
/* SYNOPSIS
/*	#include <mvect.h>
/*
/*	char	*mvect_alloc(vector, elsize, nelm, init_fn, wipe_fn)
/*	MVECT	*vector;
/*	int	elsize;
/*	int	nelm;
/*	void	(*init_fn)(char *ptr, int count);
/*	void	(*wipe_fn)(char *ptr, int count);
/*
/*	char	*mvect_realloc(vector, nelm)
/*	MVECT	*vector;
/*	int	nelm;
/*
/*	char	*mvect_free(vector)
/*	MVECT	*vector;
/* DESCRIPTION
/*	This module supports memory management for arrays of arbitrary
/*	objects.  It is up to the application to provide specific code
/*	that initializes and uses object memory.
/*
/*	mvect_alloc() initializes memory for a vector with elements
/*	of \fIelsize\fR bytes, and with at least \fInelm\fR elements.
/*	\fIinit_fn\fR is a null pointer, or a pointer to a function
/*	that initializes \fIcount\fR vector elements.
/*	\fIwipe_fn\fR is a null pointer, or a pointer to a function
/*	that is complementary to \fIinit_fn\fR. This routine is called
/*	by mvect_free(). The result of mvect_alloc() is a pointer to
/*	the allocated vector.
/*
/*	mvect_realloc() guarantees that the specified vector has space
/*	for at least \fInelm\fR elements. The result is a pointer to the
/*	allocated vector, which may change across calls.
/*
/*	mvect_free() releases storage for the named vector. The result
/*	is a convenient null pointer.
/* SEE ALSO
/*	mymalloc(3) memory management
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

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include "mymalloc.h"
#include "mvect.h"

/* mvect_alloc - allocate memory vector */

char   *mvect_alloc(MVECT *vect, int elsize, int nelm,
               void (*init_fn) (char *, int), void (*wipe_fn) (char *, int))
{
    vect->init_fn = init_fn;
    vect->wipe_fn = wipe_fn;
    vect->nelm = 0;
    vect->ptr = mymalloc(elsize * nelm);
    vect->nelm = nelm;
    vect->elsize = elsize;
    if (vect->init_fn)
	vect->init_fn(vect->ptr, vect->nelm);
    return (vect->ptr);
}

/* mvect_realloc - adjust memory vector allocation */

char   *mvect_realloc(MVECT *vect, int nelm)
{
    int     old_len = vect->nelm;
    int     incr = nelm - old_len;
    int     new_nelm;

    if (incr > 0) {
	if (incr < old_len)
	    incr = old_len;
	new_nelm = vect->nelm + incr;
	vect->ptr = myrealloc(vect->ptr, vect->elsize * new_nelm);
	vect->nelm = new_nelm;
	if (vect->init_fn)
	    vect->init_fn(vect->ptr + old_len * vect->elsize, incr);
    }
    return (vect->ptr);
}

/* mvect_free - release memory vector storage */

char   *mvect_free(MVECT *vect)
{
    if (vect->wipe_fn)
	vect->wipe_fn(vect->ptr, vect->nelm);
    myfree(vect->ptr);
    myfree((char *) vect);
    return (0);
}
