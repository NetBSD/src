/*	$NetBSD: lglob.h,v 1.1.1.2 1999/04/06 05:30:39 mrg Exp $	*/

/*
 * Copyright (c) 1984,1985,1989,1994,1995,1996,1999  Mark Nudelman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice in the documentation and/or other materials provided with 
 *    the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN 
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Macros to define the method of doing filename "globbing".
 * There are three possible mechanisms:
 *   1.	GLOB_LIST
 *	This defines a function that returns a list of matching filenames.
 *   2. GLOB_NAME
 *	This defines a function that steps thru the list of matching
 *	filenames, returning one name each time it is called.
 *   3. GLOB_STRING
 *	This defines a function that returns the complete list of
 *	matching filenames as a single space-separated string.
 */

#if OS2

#define	DECL_GLOB_LIST(list)		char **list;  char **pp;
#define	GLOB_LIST(filename,list)	list = _fnexplode(filename)
#define	GLOB_LIST_FAILED(list)		list == NULL
#define	SCAN_GLOB_LIST(list,p)		pp = list;  *pp != NULL;  pp++
#define	INIT_GLOB_LIST(list,p)		p = *pp
#define	GLOB_LIST_DONE(list)		_fnexplodefree(list)

#else
#if MSDOS_COMPILER==DJGPPC

#define	DECL_GLOB_LIST(list)		glob_t list;  int i;
#define	GLOB_LIST(filename,list)	glob(filename,GLOB_NOCHECK,0,&list)
#define	GLOB_LIST_FAILED(list)		0
#define	SCAN_GLOB_LIST(list,p)		i = 0;  i < list.gl_pathc;  i++
#define	INIT_GLOB_LIST(list,p)		p = list.gl_pathv[i]
#define	GLOB_LIST_DONE(list)		globfree(&list)

#else
#if MSDOS_COMPILER==MSOFTC || MSDOS_COMPILER==BORLANDC

#define	GLOB_FIRST_NAME(filename,fndp,h) h = _dos_findfirst(filename, ~_A_VOLID, fndp)
#define	GLOB_FIRST_FAILED(handle)	((handle) != 0)
#define	GLOB_NEXT_NAME(handle,fndp)		_dos_findnext(fndp)
#define	GLOB_NAME_DONE(handle)
#define	GLOB_NAME			name
#define	DECL_GLOB_NAME(fnd,drive,dir,fname,ext,handle) \
					struct find_t fnd;	\
					char drive[_MAX_DRIVE];	\
					char dir[_MAX_DIR];	\
					char fname[_MAX_FNAME];	\
					char ext[_MAX_EXT];	\
					int handle;
#else
#if MSDOS_COMPILER==WIN32C && defined(_MSC_VER)

#define	GLOB_FIRST_NAME(filename,fndp,h) h = _findfirst(filename, fndp)
#define	GLOB_FIRST_FAILED(handle)	((handle) == -1)
#define	GLOB_NEXT_NAME(handle,fndp)	_findnext(handle, fndp)
#define	GLOB_NAME_DONE(handle)		_findclose(handle)
#define	GLOB_NAME			name
#define	DECL_GLOB_NAME(fnd,drive,dir,fname,ext,handle) \
					struct _finddata_t fnd;	\
					char drive[_MAX_DRIVE];	\
					char dir[_MAX_DIR];	\
					char fname[_MAX_FNAME];	\
					char ext[_MAX_EXT];	\
					long handle;

#else
#if MSDOS_COMPILER==WIN32C && !defined(_MSC_VER) /* Borland C for Windows */

#define	GLOB_FIRST_NAME(filename,fndp,h) h = findfirst(filename, fndp, ~FA_LABEL)
#define	GLOB_FIRST_FAILED(handle)	((handle) != 0)
#define	GLOB_NEXT_NAME(handle,fndp)	findnext(fndp)
#define	GLOB_NAME_DONE(handle)
#define	GLOB_NAME			ff_name
#define	DECL_GLOB_NAME(fnd,drive,dir,fname,ext,handle) \
					struct ffblk fnd;	\
					char drive[MAXDRIVE];	\
					char dir[MAXDIR];	\
					char fname[MAXFILE];	\
					char ext[MAXEXT];	\
					int handle;

#endif
#endif
#endif
#endif
#endif
