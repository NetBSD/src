/* $Id: saslc.h,v 1.1.1.1 2010/11/27 21:23:58 agc Exp $ */

/* Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mateusz Kocielski.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *	  must display the following acknowledgement:
 *		  This product includes software developed by the NetBSD
 *		  Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *	  contributors may be used to endorse or promote products derived
 *	  from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.	IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SASLC_H_
#define _SASLC_H_

#include <stdlib.h>

typedef struct saslc_t saslc_t;
typedef struct saslc_sess_t saslc_sess_t;

/* begin and end */
saslc_t *saslc_alloc(void);
int saslc_init(saslc_t *, const char *);
int saslc_end(saslc_t *);

/* error */
const char *saslc_strerror(saslc_t *);
const char *saslc_sess_strerror(saslc_sess_t *);

/* session begin and end */
saslc_sess_t *saslc_sess_init(saslc_t *, const char *);
void saslc_sess_end(saslc_sess_t *);

/* session properties */
int saslc_sess_setprop(saslc_sess_t *, const char *, const char *);
const char *saslc_sess_getprop(saslc_sess_t *, const char *);
const char *saslc_sess_strmech(saslc_sess_t *);

/* session management */
int saslc_sess_cont(saslc_sess_t *, const void *, size_t, void **, size_t *);
int saslc_sess_encode(saslc_sess_t *, const void *, size_t, void **,
    size_t *);
int saslc_sess_decode(saslc_sess_t *, const void *, size_t, void **,
    size_t *);

#endif /* !_SASLC_H_ */
