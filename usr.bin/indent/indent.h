/*	$NetBSD: indent.h,v 1.2 2019/10/19 15:44:31 christos Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2001 Jens Schweikhardt
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if 0
#if defined(__NetBSD__)
__RCSID("$NetBSD: indent.h,v 1.2 2019/10/19 15:44:31 christos Exp $");
#elif defined(__FreeBSD__)
__FBSDID("$FreeBSD: head/usr.bin/indent/indent.h 336333 2018-07-16 05:46:50Z pstef $");
#endif
#endif

#ifndef nitems
#define nitems(array) (sizeof (array) / sizeof (array[0]))
#endif

void	add_typename(const char *);
void	alloc_typenames(void);
int	compute_code_target(void);
int	compute_label_target(void);
int	count_spaces(int, char *);
int	count_spaces_until(int, char *, char *);
void	init_constant_tt(void);
int	lexi(struct parser_state *);
void	diag(int, const char *, ...) __printflike(2, 3);
void	dump_line(void);
void	fill_buffer(void);
void	parse(int);
void	pr_comment(void);
void	set_defaults(void);
void	set_option(char *);
void	set_profile(const char *);
