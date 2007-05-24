/*	$NetBSD: display.h,v 1.8 2007/05/24 20:04:04 ad Exp $	*/

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

/* constants needed for display.c */

/* "type" argument for new_message function */

#define  MT_standout  1
#define  MT_delayed   2

int display_resize __P((void));
struct statics;
int display_init __P((struct statics *));
void i_loadave __P((int, double *));
void u_loadave __P((int, double *));
void i_timeofday __P((time_t *, time_t *));
void i_procstates __P((int, int *, int));
void u_procstates __P((int, int *, int));
char *cpustates_tag __P((void));
void i_cpustates __P((int *));
void u_cpustates __P((int *));
void z_cpustates __P((void));
void i_memory __P((int *));
void u_memory __P((int *));
void i_swap __P((int *));
void u_swap __P((int *));
void i_message __P((void));
void u_message __P((void));
void i_header __P((char *));
void u_header __P((char *));
void i_process __P((int, char *));
void u_process __P((int, char *));
void u_endscreen __P((int));
void display_header __P((int));
void new_message __P((int, const char *, ...))
     __attribute__((__format__(__printf__, 2, 3)));
void clear_message __P((void));
int readline __P((char *, int, int));
char *printable __P((char *));
