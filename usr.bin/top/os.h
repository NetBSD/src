/*	$NetBSD: os.h,v 1.4.10.1 2002/11/12 20:32:00 nathanw Exp $	*/

/*
 *  Top users/processes display for Unix
 *  Version 3
 *
 * Copyright (c) 1984, 1989, William LeFebvre, Rice University
 * Copyright (c) 1989, 1990, 1992, William LeFebvre, Northwestern University
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS EMPLOYER BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/param.h>	/* This defines BSD */
#if defined(BSD) && !defined(BSD4_4) && !defined(__osf__)
# include <stdio.h>
# include <strings.h>
# define strchr(a, b)		index((a), (b))
# define strrchr(a, b)		rindex((a), (b))
# define memcpy(a, b, c)	bcopy((b), (a), (c))
# define memzero(a, b)		bzero((a), (b))
# define memcmp(a, b, c)	bcmp((a), (b), (c))
#if defined(NeXT)
  typedef void sigret_t;
#else
  typedef int sigret_t;
#endif

/* system routines that don't return int */
char *getenv();
caddr_t malloc();

#else 
# include <stdio.h>
# define setbuffer(f, b, s)	setvbuf((f), (b), (b) ? _IOFBF : _IONBF, (s))
# include <string.h>
# include <memory.h>
# include <stdlib.h>
# include <unistd.h>
# define memzero(a, b)		memset((a), 0, (b))
  typedef void sigret_t;
#endif

/* some systems declare sys_errlist in stdio.h! */
#if defined(__NetBSD__) || defined(__FreeBSD__)
#define SYS_ERRLIST_DECLARED
#endif
