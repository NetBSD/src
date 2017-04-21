/*	$NetBSD: load_lib.c,v 1.2.4.2 2017/04/21 16:52:53 bouyer Exp $	*/

/*++
/* NAME
/*	load_lib 3
/* SUMMARY
/*	library loading wrappers
/* SYNOPSIS
/*	#include <load_lib.h>
/*
/*	void	load_library_symbols(const char *, LIB_FN *, LIB_DP *);
/*	const char *libname;
/*	LIB_FN	*libfuncs;
/*	LIB_DP	*libdata;
/* DESCRIPTION
/*	load_library_symbols() loads the specified shared object
/*	and looks up the function or data pointers for the specified
/*	symbols. All errors are fatal.
/*
/*	Arguments:
/* .IP libname
/*	shared-library pathname.
/* .IP libfuncs
/*	Array of LIB_FN strucures. The last name member must be null.
/* .IP libdata
/*	Array of LIB_DP strucures. The last name member must be null.
/* SEE ALSO
/*	msg(3) diagnostics interface
/* DIAGNOSTICS
/*	Problems are reported via the msg(3) diagnostics routines:
/*	library not found, symbols not found, other fatal errors.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	LaMont Jones
/*	Hewlett-Packard Company
/*	3404 Harmony Road
/*	Fort Collins, CO 80528, USA
/*
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

 /*
  * System libraries.
  */
#include "sys_defs.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#ifdef USE_DYNAMIC_MAPS
#if defined(HAS_DLOPEN)
#include <dlfcn.h>
#elif defined(HAS_SHL_LOAD)
#include <dl.h>
#else
#error "USE_DYNAMIC_LIBS requires HAS_DLOPEN or HAS_SHL_LOAD"
#endif

 /*
  * Utility library.
  */
#include <msg.h>
#include <load_lib.h>

/* load_library_symbols - load shared library and look up symbols */

void    load_library_symbols(const char *libname, LIB_FN *libfuncs,
			             LIB_DP *libdata)
{
    static const char myname[] = "load_library_symbols";
    LIB_FN *fn;
    LIB_DP *dp;

#if defined(HAS_DLOPEN)
    void   *handle;
    char   *emsg;

    /*
     * XXX This is basically how FreeBSD dlfunc() silences a compiler warning
     * about a data/function pointer conversion. The solution below is non-
     * portable: it assumes that both data and function pointers are the same
     * in size, and that both have the same representation.
     */
    union {
	void   *dptr;			/* data pointer */
	void    (*fptr) (void);		/* function pointer */
    }       non_portable_union;

    if ((handle = dlopen(libname, RTLD_NOW)) == 0) {
	emsg = dlerror();
	msg_fatal("%s: dlopen failure loading %s: %s", myname, libname,
		  emsg ? emsg : "don't know why");
    }
    if (libfuncs) {
	for (fn = libfuncs; fn->name; fn++) {
	    if ((non_portable_union.dptr = dlsym(handle, fn->name)) == 0) {
		emsg = dlerror();
		msg_fatal("%s: dlsym failure looking up %s in %s: %s", myname,
			  fn->name, libname, emsg ? emsg : "don't know why");
	    }
	    fn->fptr = non_portable_union.fptr;
	    if (msg_verbose > 1)
		msg_info("loaded %s = %p", fn->name, non_portable_union.dptr);
	}
    }
    if (libdata) {
	for (dp = libdata; dp->name; dp++) {
	    if ((dp->dptr = dlsym(handle, dp->name)) == 0) {
		emsg = dlerror();
		msg_fatal("%s: dlsym failure looking up %s in %s: %s", myname,
			  dp->name, libname, emsg ? emsg : "don't know why");
	    }
	    if (msg_verbose > 1)
		msg_info("loaded %s = %p", dp->name, dp->dptr);
	}
    }
#elif defined(HAS_SHL_LOAD)
    shl_t   handle;

    handle = shl_load(libname, BIND_IMMEDIATE, 0);

    if (libfuncs) {
	for (fn = libfuncs; fn->name; fn++) {
	    if (shl_findsym(&handle, fn->name, TYPE_PROCEDURE, &fn->fptr) != 0)
		msg_fatal("%s: shl_findsym failure looking up %s in %s: %m",
			  myname, fn->name, libname);
	    if (msg_verbose > 1)
		msg_info("loaded %s = %p", fn->name, (void *) fn->fptr);
	}
    }
    if (libdata) {
	for (dp = libdata; dp->name; dp++) {
	    if (shl_findsym(&handle, dp->name, TYPE_DATA, &dp->dptr) != 0)
		msg_fatal("%s: shl_findsym failure looking up %s in %s: %m",
			  myname, dp->name, libname);
	    if (msg_verbose > 1)
		msg_info("loaded %s = %p", dp->name, dp->dptr);
	}
    }
#endif
}

#endif
