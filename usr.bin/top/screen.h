/*	$NetBSD: screen.h,v 1.5.6.1 2002/11/12 20:32:01 nathanw Exp $	*/

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

/*
 *  top - a top users display for Unix 4.2
 *
 *  This file contains all the definitions necessary to use the hand-written
 *  screen package in "screen.c"
 */

#define TCputs(str)	tputs(str, 1, putstdout)
#define putcap(str)	(void)((str) != NULL ? TCputs(str) : 0)

/* declare return values for termcap functions */
#ifdef __NetBSD__
#include <termcap.h>
#else
char *tgetstr();
char *tgoto();
#endif

extern char ch_erase;		/* set to the user's erase character */
extern char ch_kill;		/* set to the user's kill  character */
extern char smart_terminal;     /* set if the terminal has sufficient termcap
				   capabilities for normal operation */

/* These are some termcap strings for use outside of "screen.c" */
extern char *cursor_motion;
extern char *clear_line;
extern char *clear_to_end;

/* rows and columns on the screen according to termcap */
extern int  screen_length;
extern int  screen_width;

void init_termcap __P((int));
void init_screen __P((void));
void end_screen __P((void));
void reinit_screen __P((void));
void get_screensize __P((void));
void standout __P((char *));
void clear __P((void));
int clear_eol __P((int));
void go_home __P((void));
int putstdout __P((int));
void Move_to(int x, int y);
