/*	$NetBSD: specificdata.h,v 1.2 2006/10/11 05:37:32 thorpej Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _SYS_SPECIFICDATA_H_
#define	_SYS_SPECIFICDATA_H_

#include <sys/lock.h>

typedef unsigned int specificdata_key_t;
typedef void (*specificdata_dtor_t)(void *);
typedef struct specificdata_domain *specificdata_domain_t;
typedef struct specificdata_container *specificdata_container_t;

typedef struct {
	specificdata_container_t specdataref_container;
	struct simplelock specdataref_slock;
} specificdata_reference;

specificdata_domain_t	specificdata_domain_create(void);
void			specificdata_domain_delete(specificdata_domain_t);

int	specificdata_key_create(specificdata_domain_t,
				specificdata_key_t *, specificdata_dtor_t);
void	specificdata_key_delete(specificdata_domain_t, specificdata_key_t);

int	specificdata_init(specificdata_domain_t, specificdata_reference *);
void	specificdata_fini(specificdata_domain_t, specificdata_reference *);

void *	specificdata_getspecific(specificdata_domain_t,
				 specificdata_reference *, specificdata_key_t);
void *	specificdata_getspecific_unlocked(specificdata_domain_t,
					  specificdata_reference *,
					  specificdata_key_t);
void	specificdata_setspecific(specificdata_domain_t,
				 specificdata_reference *, specificdata_key_t,
				 void *);
int	specificdata_setspecific_nowait(specificdata_domain_t,
					specificdata_reference *,
					specificdata_key_t, void *);

#endif /* _SYS_SPECIFICDATA_H_ */
