/*	$NetBSD: search.c,v 1.2 1997/02/03 19:45:02 cgd Exp $	*/

/*
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by John Polstra.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Dynamic linker for ELF.
 *
 * John Polstra <jdp@polstra.com>.
 */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>

#include "debug.h"
#include "rtld.h"

#define CONCAT(x,y)     __CONCAT(x,y)
#define ELFNAME(x)      CONCAT(elf,CONCAT(ELFSIZE,CONCAT(_,x)))
#define ELFNAME2(x,y)   CONCAT(x,CONCAT(_elf,CONCAT(ELFSIZE,CONCAT(_,y))))
#define ELFNAMEEND(x)   CONCAT(x,CONCAT(_elf,ELFSIZE))
#define ELFDEFNNAME(x)  CONCAT(ELF,CONCAT(ELFSIZE,CONCAT(_,x)))

/*
 * Data declarations.
 */

typedef struct {
    const char *si_name;
    const char *si_best_name;
    char *si_best_fullpath;
    const Search_Path *si_best_path;
    size_t si_namelen;
    int si_desired_major;
    int si_desired_minor;
    int si_best_major;
    int si_best_minor;
    unsigned si_exact : 1;
} Search_Info;

typedef enum {
    Search_FoundNothing,
    Search_FoundLower,
    Search_FoundHigher,
    Search_FoundExact
} Search_Result; 

static bool
_rtld_check_library(
    const Search_Path *sp,
    const char *name,
    size_t namelen,
    char **fullpath_p)
{
    struct stat mystat;
    char *fullpath;
    Elf_Ehdr ehdr;
    int fd;

    fullpath = xmalloc(sp->sp_pathlen + 1 + namelen + 1);
    strncpy(fullpath, sp->sp_path, sp->sp_pathlen);
    fullpath[sp->sp_pathlen] = '/';
    strcpy(&fullpath[sp->sp_pathlen + 1], name);

    dbg("  Trying \"%s\"", fullpath);
    if (stat(fullpath, &mystat) >= 0 && S_ISREG(mystat.st_mode)) {
	if ((fd = open(fullpath, O_RDONLY)) >= 0) {
	    if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr))
		goto lose;

	    /* Elf_e_ident includes class */
	    if (memcmp(Elf_e_ident, ehdr.e_ident, Elf_e_siz) != 0)
		goto lose;

	    switch (ehdr.e_machine) {
	    ELFDEFNNAME(MACHDEP_ID_CASES) 

	    default:
		goto lose;
	    }

	    if (ehdr.e_ident[Elf_ei_version] != Elf_ev_current ||
	      ehdr.e_version != Elf_ev_current ||
	      ehdr.e_ident[Elf_ei_data] != ELFDEFNNAME(MACHDEP_ENDIANNESS) ||
	      ehdr.e_type != Elf_et_dyn)
		goto lose;

	    if (*fullpath_p != NULL)
		free(*fullpath_p);
	    *fullpath_p = fullpath;
	    close(fd);
	    return true;

lose:
	    close(fd);
	}
    }

    free(fullpath);
    return false;
}

static Search_Result
_rtld_search_directory(
    const Search_Path *sp,
    Search_Info *si)
{
    struct dirent *entry;
    DIR *dirp;
    Search_Result result = Search_FoundNothing;

dbg("_rtld_search_directory");
    if (sp->sp_path == NULL || sp->sp_path[0] == '\0')
	return result;

dbg("_rtld_search_directory 2");
    if ((dirp = opendir(sp->sp_path)) == NULL) {
	dbg("_rtld_search_directory 2.1");
	return result;
    }

dbg("_rtld_search_directory 3");
    while ((entry = readdir(dirp)) != NULL) {
	long major = -1;
	long minor = -1;
	if (strncmp(entry->d_name, si->si_name, si->si_namelen))
	    continue;
	/*
	 * We are matching libfoo.so only (no more info).  Only take
	 * it as a last resort.
	 */
	if (si->si_exact) {
	    if (strcmp(entry->d_name, si->si_name))
		continue;
#ifdef notyet
	} else if (entry->d_namlen == si->si_namelen) {
	    if (si->si_best_path != NULL || si->si_best_major != -1)
		continue;
#endif
	} else {
	    char *cp;
	    /*
	     * We expect (demand!) that it be of the form
	     * "libfoo.so.<something>"
	     */
	    if (entry->d_name[si->si_namelen] != '.')
		continue;
	    /*
	     * This file has a least a major number (well, maybe not if it
	     * has a name of "libfoo.so." but treat that as equivalent to 0.
	     * It had better match what we are looking for.
	     */
	    major = strtol(&entry->d_name[si->si_namelen+1], &cp, 10);
	    if (major < 0 || (cp[0] != '\0' && cp[0] != '.')
		    || &entry->d_name[si->si_namelen+1] == cp)
		continue;
	    if (cp[0] == '.') {
		char *cp2;
		minor = strtol(&cp[1], &cp2, 10);
		if (minor < 0 || cp2[0] != '\0' || cp == cp2)
		    continue;
	    } else {
		minor = 0;
	    }
	    if (major != si->si_desired_major || minor <= si->si_best_minor)
		continue;
	}
	/*
	 * We have a better candidate...
	 */
	if (!_rtld_check_library(sp, entry->d_name, entry->d_namlen,
			   &si->si_best_fullpath))
	    continue;
	    
	si->si_best_name = &si->si_best_fullpath[sp->sp_pathlen + 1];
	si->si_best_major = major;
	si->si_best_minor = minor;
	si->si_best_path = sp;

	if (si->si_exact || si->si_best_minor == si->si_desired_minor)
	    result = Search_FoundExact;
	else if (si->si_best_minor > si->si_desired_minor)
	    result = Search_FoundHigher;
	else
	    result = Search_FoundLower;

	/*
	 * We were looking for, and found, an exact match.  We're done.
	 */
	if (si->si_exact)
	    break;
    }

    dbg("found %s (%d.%d) match for %s (%d.%d) -> %s",
	result == Search_FoundNothing ? "no"
	: result == Search_FoundLower ? "lower"
	: result == Search_FoundExact ? "exact" : "higher",
	si->si_best_major, si->si_best_minor,
	si->si_name,
	si->si_desired_major, si->si_desired_minor,
	si->si_best_fullpath ? si->si_best_fullpath : sp->sp_path);

    closedir(dirp);
    return result;
}

static char *
_rtld_search_library_paths(
    const char *name,
    Search_Path *paths,
    const Search_Path *rpaths)
{
    Search_Info info;
    Search_Path *path;
    const char *cp;
    Search_Result result = Search_FoundNothing;

    memset(&info, 0, sizeof(info));
    info.si_name = name;
    info.si_desired_major = -1;
    info.si_desired_minor = -1;
    info.si_best_major = -1;
    info.si_best_minor = -1;

    cp = strstr(name, ".so");
    if (cp == NULL) {
	info.si_exact = true;
    } else {
	cp += sizeof(".so") - 1;
	info.si_namelen = cp - name;
	if (cp[0] != '.') {
	    info.si_exact = true;
	} else {
	    info.si_desired_major = atoi(&cp[1]);
	    if ((cp = strchr(&cp[1], '.')) != NULL) {
		info.si_desired_minor = atoi(&cp[1]);
	    } else {
		info.si_desired_minor = 0;
	    }
	}
    }

    if (rpaths != NULL && result < Search_FoundHigher) {	/* Exact? */
	dbg("  checking rpaths..");
	for (; rpaths != NULL; rpaths = rpaths->sp_next) {
	    dbg("   in \"%s\"", rpaths->sp_path);
	    result = _rtld_search_directory(rpaths, &info);
	    if (result >= Search_FoundHigher)	/* Exact? */
		break;
	}
    }
    if (result < Search_FoundHigher) {	/* Exact? */
	dbg("  checking default paths..");
	for (path = paths; path != NULL; path = path->sp_next) {
	    dbg("   in \"%s\"", path->sp_path);
	    result = _rtld_search_directory(path, &info);
	    if (result >= Search_FoundHigher)	/* Exact? */
		break;
	}
    }

    if (result >= Search_FoundHigher)
	return info.si_best_fullpath;

    if (info.si_best_fullpath != NULL)
	free(info.si_best_fullpath);
    return NULL;
}

/*
 * Find the library with the given name, and return its full pathname.
 * The returned string is dynamically allocated.  Generates an error
 * message and returns NULL if the library cannot be found.
 *
 * If the second argument is non-NULL, then it refers to an already-
 * loaded shared object, whose library search path will be searched.
 */
char *
_rtld_find_library(
    const char *name,
    const Obj_Entry *refobj)
{
    char *pathname;

    if (strchr(name, '/') != NULL) {	/* Hard coded pathname */
	if (name[0] != '/' && !_rtld_trust) {
	    _rtld_error("Absolute pathname required for shared object \"%s\"",
			name);
	    return NULL;
	}
#ifdef SVR4_LIBDIR
	if (strncmp(name, SVR4_LIBDIR, SVR4_LIBDIRLEN) == 0
	        && name[SVR4_LIBDIRLEN] == '/') {	/* In "/usr/lib" */
	    /* Map hard-coded "/usr/lib" onto our ELF library directory. */
	    pathname = xmalloc(strlen(name) + LIBDIRLEN - SVR4_LIBDIRLEN + 1);
	    strcpy(pathname, LIBDIR);
	    strcpy(pathname + LIBDIRLEN, name + SVR4_LIBDIRLEN);
	    return pathname;
	}
#endif /* SVR4_LIBDIR */
	return xstrdup(name);
    }

    dbg(" Searching for \"%s\" (%p)", name, refobj);

    pathname = _rtld_search_library_paths(name, _rtld_paths, 
					  refobj ? refobj->rpaths : NULL);
    if (pathname == NULL)
	_rtld_error("Shared object \"%s\" not found", name);
    return pathname;
}
