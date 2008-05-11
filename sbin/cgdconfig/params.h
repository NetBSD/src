/* $NetBSD: params.h,v 1.10 2008/05/11 03:15:21 elric Exp $ */

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

#ifndef PARAMS_H
#define PARAMS_H

#include "utils.h"

struct keygen {
	int		 kg_method;
	size_t		 kg_iterations;
	bits_t		*kg_salt;
	bits_t		*kg_key;
	string_t	*kg_cmd;
	struct keygen	*next;
};

struct params {
	string_t	*algorithm;
	string_t	*ivmeth;
	bits_t		*key;
	size_t		 keylen;
	size_t		 bsize;
	int		 verify_method;
	struct keygen	*dep_keygen;
	struct keygen	*keygen;
};

/* key generation methods */

#define KEYGEN_UNKNOWN			0x0
#define KEYGEN_RANDOMKEY		0x1
#define KEYGEN_PKCS5_PBKDF2_OLD		0x2
#define KEYGEN_STOREDKEY		0x3
#define KEYGEN_URANDOMKEY		0x4
#define KEYGEN_PKCS5_PBKDF2_SHA1	0x5
#define KEYGEN_SHELL_CMD		0x6

/* verification methods */

#define VERIFY_UNKNOWN		0x0
#define VERIFY_NONE		0x1
#define VERIFY_DISKLABEL	0x2
#define VERIFY_FFS		0x3
#define VERIFY_REENTER		0x4

__BEGIN_DECLS
struct params	*params_new(void);
void		 params_free(struct params *);

int		 params_filldefaults(struct params *);
int		 params_verify(const struct params *);

struct params	*params_combine(struct params *, struct params *);
struct params	*params_algorithm(string_t *);
struct params	*params_ivmeth(string_t *);
struct params	*params_keylen(size_t);
struct params	*params_bsize(size_t);
struct params	*params_verify_method(string_t *);
struct params	*params_keygen(struct keygen *);
struct params	*params_dep_keygen(struct keygen *);

struct params	*params_fget(FILE *);
struct params	*params_cget(const char *);
int		 params_fput(struct params *, FILE *);
int		 params_cput(struct params *, const char *);

struct keygen	*keygen_new(void);
void		 keygen_free(struct keygen *);

int		 keygen_filldefaults(struct keygen *, size_t);
int		 keygen_verify(const struct keygen *);
void		 keygen_addlist(struct keygen **, struct keygen *);

struct keygen	*keygen_combine(struct keygen *, struct keygen *);
struct keygen	*keygen_generate(int);
struct keygen	*keygen_method(string_t *);
struct keygen	*keygen_set_method(struct keygen *, string_t *);
struct keygen	*keygen_salt(bits_t *);
struct keygen	*keygen_iterations(size_t);
struct keygen	*keygen_key(bits_t *);
struct keygen	*keygen_cmd(string_t *);

int		 keygen_fput(struct keygen *, int, FILE *);
__END_DECLS

#endif
