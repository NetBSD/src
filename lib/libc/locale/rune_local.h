/*	$NetBSD: rune_local.h,v 1.5.2.1 2002/03/22 20:42:18 nathanw Exp $	*/

/*-
 * Copyright (c) 2000 Citrus Project,
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _RUNE_LOCAL_H_
#define _RUNE_LOCAL_H_

/* rune.c */
extern _RuneLocale *_Read_RuneMagi __P((FILE *fp));
extern _RuneLocale *_Read_CTypeAsRune __P((FILE *fp));
extern void _NukeRune __P((_RuneLocale *));

/* setrunelocale.c */
extern int _xpg4_setrunelocale __P((char *));
extern _RuneLocale *_findrunelocale __P((char *));
extern int _newrunelocale __P((char *));

/* runeglue.c */
extern int __runetable_to_netbsd_ctype __P((const char *));

/* ___runetype_mb.c */
extern unsigned long ___runetype_mb __P((wint_t));

/* ___tolower_mb.c */
extern wint_t ___tolower_mb __P((wint_t));

/* ___toupper_mb.c */
extern wint_t ___toupper_mb __P((wint_t));

#endif
