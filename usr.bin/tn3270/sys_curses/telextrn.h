/*	$NetBSD: telextrn.h,v 1.5 1998/03/04 13:16:09 christos Exp $	*/

/*-
 * Copyright (c) 1988 The Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	from: @(#)telextrn.h	4.2 (Berkeley) 4/26/91
 */

/*
 * Definitions of external routines and variables for tn3270
 */

/*
 * Pieces exported from the telnet susbsection.
 */

extern int
#if defined(unix)
	HaveInput,
#endif /* defined(unix) */
	tout,
	tin;

extern char	*transcom;

/* system.c */
void freestorage __P((void));
void movetous __P((char *, unsigned int, unsigned int , int));
void movetothem __P((unsigned int, unsigned int , char *, int));
char *access_api __P((char *, int, int ));
void unaccess_api __P((char *, char *, int, int));
int shell_continue __P((void));
int shell __P((int, char *[]));

/* termout.c */
void init_screen __P((void));
void InitTerminal __P((void));
void StopScreen __P((int));
void RefreshScreen __P((void));
void ConnectScreen __P((void));
void LocalClearScreen __P((void));
void BellOff __P((void));
void RingBell __P((char *));
int DoTerminalOutput __P((void));
void TransStop __P((void));
void TransOut __P((unsigned char *, int, int, int));
