/* $NetBSD: utils.h,v 1.3 2003/09/23 17:24:46 cb Exp $ */

/*-
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland C. Dowdeswell.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef CGDUTILS_H
#define CGDUTILS_H

#include <stdio.h>

#define BITS2BYTES(n)	(((n) + 7) / 8)

struct string;
typedef struct string string_t;

struct bits;
typedef struct bits bits_t;

__BEGIN_DECLS
char		**words(const char *, int *);
void	  	  words_free(char **, int);

void		  memxor(void *, const void *, size_t);

void		  free_notnull(void *);

string_t	 *string_new(const char *, int);
string_t	 *string_dup(const string_t *);
void		  string_assign(string_t **, string_t *);
void		  string_free(string_t *);
string_t	 *string_add(const string_t *, const string_t *);
string_t	 *string_add_d(string_t *, string_t *);
string_t	 *string_fromcharstar(const char *);
const char	 *string_tocharstar(const string_t *);
string_t	 *string_fromint(int);

void		  string_fprint(FILE *, const string_t *);

bits_t		 *bits_new(const void *, int);
bits_t		 *bits_dup(const bits_t *);
void		  bits_assign(bits_t **, bits_t *);
void		  bits_free(bits_t *);
const void	 *bits_getbuf(bits_t *);
int		  bits_len(bits_t *);
int		  bits_match(const bits_t *, const bits_t *);

bits_t		 *bits_xor(const bits_t *, const bits_t *);
bits_t		 *bits_xor_d(bits_t *, bits_t *);

bits_t		 *bits_decode(const string_t *);
bits_t		 *bits_decode_d(string_t *);
string_t	 *bits_encode(const bits_t *);
string_t	 *bits_encode_d(bits_t *);

bits_t		 *bits_cget(const char *, int);
bits_t		 *bits_fget(FILE *, int);
bits_t		 *bits_getrandombits(int);

void		  bits_fprint(FILE *, const bits_t *);
__END_DECLS

#endif
