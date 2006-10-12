/*	$NetBSD: prop_number.h,v 1.4 2006/10/12 04:46:56 thorpej Exp $	*/

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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

#ifndef _PROPLIB_PROP_NUMBER_H_
#define	_PROPLIB_PROP_NUMBER_H_

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <stdint.h>
#endif
#include <prop/prop_object.h>

typedef struct _prop_number *prop_number_t;

__BEGIN_DECLS
prop_number_t	prop_number_create_integer(int64_t);
prop_number_t	prop_number_create_unsigned_integer(uint64_t);

prop_number_t	prop_number_copy(prop_number_t);

int		prop_number_size(prop_number_t);
boolean_t	prop_number_unsigned(prop_number_t);

int64_t		prop_number_integer_value(prop_number_t);
uint64_t	prop_number_unsigned_integer_value(prop_number_t);

boolean_t	prop_number_equals(prop_number_t, prop_number_t);
boolean_t	prop_number_equals_integer(prop_number_t, int64_t);
boolean_t	prop_number_equals_unsigned_integer(prop_number_t, uint64_t);
__END_DECLS

#endif /* _PROPLIB_PROP_NUMBER_H_ */
