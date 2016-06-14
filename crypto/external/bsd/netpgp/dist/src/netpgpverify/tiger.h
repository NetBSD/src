/*-
 * Copyright (c) 2005-2011 Alistair Crooks <agc@NetBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef TIGER_H_
#define TIGER_H_

#include <sys/types.h>

#include <inttypes.h>

#ifndef __BEGIN_DECLS
#  if defined(__cplusplus)
#  define __BEGIN_DECLS           extern "C" {
#  define __END_DECLS             }
#  else
#  define __BEGIN_DECLS
#  define __END_DECLS
#  endif
#endif

__BEGIN_DECLS

#define TIGER_DIGEST_LENGTH            24
#define TIGER_DIGEST_STRING_LENGTH     ((TIGER_DIGEST_LENGTH * 2) + 1)

typedef struct NETPGPV_TIGER_CTX {
	uint64_t	ctx[3];
	int		init;
	uint8_t		pad;
} NETPGPV_TIGER_CTX;

void netpgpv_TIGER_Init(NETPGPV_TIGER_CTX *);
void netpgpv_TIGER2_Init(NETPGPV_TIGER_CTX *);
void netpgpv_TIGER_Update(NETPGPV_TIGER_CTX *, const void *, size_t);
void netpgpv_TIGER_Final(uint8_t *, NETPGPV_TIGER_CTX *);

char *netpgpv_TIGER_End(NETPGPV_TIGER_CTX *, char *);

char *netpgpv_TIGER_File(char *, char *, int);
char *netpgpv_TIGER_Data(const uint8_t *, size_t, char *, int);

__END_DECLS

#endif
