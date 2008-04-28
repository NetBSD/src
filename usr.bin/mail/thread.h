/*	$NetBSD: thread.h,v 1.2 2008/04/28 20:24:14 martin Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Anon Ymous.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef THREAD_SUPPORT

#ifndef __THREAD_H__
#define __THREAD_H__

#if 0
#define NDEBUG	/* disable the asserts */
#endif

/*
 * The core message control routines.  Without thread support, they
 * live in fio.c.
 */
struct message *next_message(struct message *);
struct message *prev_message(struct message *);
struct message *get_message(int);
int	 get_msgnum(struct message *);
int	 get_msgCount(void);

/* These give special access to the message array needed in lex.c */
struct message *get_abs_message(int);
struct message *next_abs_message(struct message *);
int	 get_abs_msgCount(void);

/*
 * Support hooks used by other modules.
 */
void	 thread_fix_old_links(struct message *, struct message *, int);
void	 thread_fix_new_links(struct message *, int, int);
int	 thread_hidden(void);
int	 thread_depth(void);
int	 do_recursion(void);
int	 thread_recursion(struct message *, int (*)(struct message *, void *), void *);
const char *thread_next_key_name(const void **);

/*
 * Commands.
 */
/* thread setup */
int	 flattencmd(void *);
int	 reversecmd(void *v);
int	 sortcmd(void *);
int	 threadcmd(void *);
int	 unthreadcmd(void *);

/* thread navigation */
int	 downcmd(void *);
int	 tsetcmd(void *);
int	 upcmd(void *);

/* thread display */
int	 exposecmd(void *);
int	 hidecmd(void *);

/* tag commands */
int	 invtagscmd(void *);
int	 tagbelowcmd(void *);
int	 tagcmd(void *);
int	 untagcmd(void *);

/* tag display */
int	 hidetagscmd(void *);
int	 showtagscmd(void *);

/* something special */
int	 deldupscmd(void *);

#define ENAME_RECURSIVE_CMDS	"recursive-commands"

/*
 * Debugging stuff that should go away.
 */
#define THREAD_DEBUG
#ifdef THREAD_DEBUG
int	 thread_showcmd(void *);
#endif /* THREAD_DEBUG */

#endif /* __THREAD_H__ */
#endif /* THREAD_SUPPORT */
