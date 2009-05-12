/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alistair Crooks (agc@NetBSD.org)
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
/*
 * Copyright (c) 2005-2008 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer. The Contributors have asserted
 * their moral rights under the UK Copyright Design and Patents Act 1988 to
 * be recorded as the authors of this copyright work.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** \file
 */

#include "packet.h"

#define DECLARE_ARRAY(type,arr)	\
	unsigned n##arr; unsigned n##arr##_allocated; type *arr

#define EXPAND_ARRAY(str, arr) do {					\
	if (str->n##arr == str->n##arr##_allocated) {			\
		str->n##arr##_allocated=str->n##arr##_allocated*2+10;	\
		str->arr=realloc(str->arr,				\
			str->n##arr##_allocated*sizeof(*str->arr));	\
	}								\
} while(/*CONSTCOND*/0)

/** __ops_keydata_key_t
 */
typedef union {
	__ops_pubkey_t pubkey;
	__ops_seckey_t seckey;
}               __ops_keydata_key_t;


/** sigpacket_t */
typedef struct {
	__ops_user_id_t		*userid;
	__ops_subpacket_t	*packet;
}               sigpacket_t;

/* XXX: gonna have to expand this to hold onto subkeys, too... */
/** \struct __ops_keydata
 * \todo expand to hold onto subkeys
 */
struct __ops_keydata {
	DECLARE_ARRAY(__ops_user_id_t, uids);
	DECLARE_ARRAY(__ops_subpacket_t, packets);
	DECLARE_ARRAY(sigpacket_t, sigs);
	unsigned char		key_id[8];
	__ops_fingerprint_t	fingerprint;
	__ops_content_tag_t	type;
	__ops_keydata_key_t	key;
};
