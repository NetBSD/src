/*	$NetBSD: glob.h,v 1.6 2003/08/07 11:14:38 agc Exp $	*/

/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)glob.h	8.1 (Berkeley) 6/6/93
 *	$NetBSD: glob.h,v 1.6 2003/08/07 11:14:38 agc Exp $
 */

/*
 * A bunch of global variable declarations lie herein.
 * def.h must be included first.
 */

#ifndef EXTERN
#define EXTERN extern
#endif

EXTERN int	msgCount;			/* Count of messages read in */
EXTERN int	rcvmode;			/* True if receiving mail */
EXTERN int	sawcom;				/* Set after first command */
EXTERN char	*Tflag;				/* -T temp file for netnews */
EXTERN int	senderr;			/* An error while checking */
EXTERN int	edit;				/* Indicates editing a file */
EXTERN int	readonly;			/* Will be unable to rewrite file */
EXTERN int	noreset;			/* String resets suspended */
EXTERN int	sourcing;			/* Currently reading variant file */
EXTERN int	loading;			/* Loading user definitions */
EXTERN int	cond;				/* Current state of conditional exc. */
EXTERN FILE	*itf;				/* Input temp file buffer */
EXTERN FILE	*otf;				/* Output temp file buffer */
EXTERN int	image;				/* File descriptor for image of msg */
EXTERN FILE	*input;				/* Current command input file */
EXTERN char	mailname[PATHSIZE];		/* Name of current file */
EXTERN char	prevfile[PATHSIZE];		/* Name of previous file */
EXTERN char	*homedir;			/* Path name of home directory */
EXTERN char	*myname;			/* My login name */
EXTERN off_t	mailsize;			/* Size of system mailbox */
EXTERN int	lexnumber;			/* Number of TNUMBER from scan() */
EXTERN char	lexstring[STRINGLEN];		/* String from TSTRING, scan() */
EXTERN int	regretp;			/* Pointer to TOS of regret tokens */
EXTERN int	regretstack[REGDEP];		/* Stack of regretted tokens */
EXTERN char	*string_stack[REGDEP];		/* Stack of regretted strings */
EXTERN int	numberstack[REGDEP];		/* Stack of regretted numbers */
EXTERN struct	message	*dot;			/* Pointer to current message */
EXTERN struct	message	*message;		/* The actual message structure */
EXTERN struct	var	*variables[HSHSIZE];	/* Pointer to active var list */
EXTERN struct	grouphead	*groups[HSHSIZE];/* Pointer to active groups */
EXTERN struct	ignoretab	ignore[2];	/* ignored and retained fields
						   0 is ignore, 1 is retain */
EXTERN struct	ignoretab	saveignore[2];	/* ignored and retained fields
						   on save to folder */
EXTERN struct	ignoretab	ignoreall[2];	/* special, ignore all headers */
EXTERN char	**altnames;			/* List of alternate names for user */
EXTERN int	debug;				/* Debug flag set */
EXTERN int	screenwidth;			/* Screen width, or best guess */
EXTERN int	screenheight;			/* Screen height, or best guess,
						   for "header" command */
EXTERN int	realscreenheight;		/* the real screen height */

#include <setjmp.h>

EXTERN jmp_buf	srbuf;


/*
 * The pointers for the string allocation routines,
 * there are NSPACE independent areas.
 * The first holds STRINGSIZE bytes, the next
 * twice as much, and so on.
 */

#define	NSPACE	25			/* Total number of string spaces */
EXTERN struct strings {
	char	*s_topFree;		/* Beginning of this area */
	char	*s_nextFree;		/* Next alloctable place here */
	unsigned s_nleft;		/* Number of bytes left here */
} stringdope[NSPACE];
