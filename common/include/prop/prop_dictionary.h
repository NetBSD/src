/*	$NetBSD: prop_dictionary.h,v 1.5 2006/09/22 04:20:23 thorpej Exp $	*/

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

#ifndef _PROPLIB_PROP_DICTIONARY_H_
#define	_PROPLIB_PROP_DICTIONARY_H_

#include <prop/prop_object.h>

typedef struct _prop_dictionary *prop_dictionary_t;
typedef struct _prop_dictionary_keysym *prop_dictionary_keysym_t;

__BEGIN_DECLS
prop_dictionary_t prop_dictionary_create(void);
prop_dictionary_t prop_dictionary_create_with_capacity(unsigned int);

prop_dictionary_t prop_dictionary_copy(prop_dictionary_t);
prop_dictionary_t prop_dictionary_copy_mutable(prop_dictionary_t);

unsigned int	prop_dictionary_capacity(prop_dictionary_t);
unsigned int	prop_dictionary_count(prop_dictionary_t);
boolean_t	prop_dictionary_ensure_capacity(prop_dictionary_t,
						unsigned int);

void		prop_dictionary_make_immutable(prop_dictionary_t);
boolean_t	prop_dictionary_mutable(prop_dictionary_t);

prop_object_iterator_t prop_dictionary_iterator(prop_dictionary_t);

prop_object_t	prop_dictionary_get(prop_dictionary_t, const char *);
boolean_t	prop_dictionary_set(prop_dictionary_t, const char *,
				    prop_object_t);
void		prop_dictionary_remove(prop_dictionary_t, const char *);

prop_object_t	prop_dictionary_get_keysym(prop_dictionary_t,
					   prop_dictionary_keysym_t);
boolean_t	prop_dictionary_set_keysym(prop_dictionary_t,
					   prop_dictionary_keysym_t,
					   prop_object_t);
void		prop_dictionary_remove_keysym(prop_dictionary_t,
					      prop_dictionary_keysym_t);

boolean_t	prop_dictionary_equals(prop_dictionary_t, prop_dictionary_t);

char *		prop_dictionary_externalize(prop_dictionary_t);
prop_dictionary_t prop_dictionary_internalize(const char *);

boolean_t	prop_dictionary_externalize_to_file(prop_dictionary_t,
						    const char *);
prop_dictionary_t prop_dictionary_internalize_from_file(const char *);

const char *	prop_dictionary_keysym_cstring_nocopy(prop_dictionary_keysym_t);

boolean_t	prop_dictionary_keysym_equals(prop_dictionary_keysym_t,
					      prop_dictionary_keysym_t);

#if defined(__NetBSD__)
#if !defined(_KERNEL) && !defined(_STANDALONE)
int		prop_dictionary_send_ioctl(prop_dictionary_t, int,
					   unsigned long);
int		prop_dictionary_recv_ioctl(int, unsigned long,
					   prop_dictionary_t *);
int		prop_dictionary_sendrecv_ioctl(prop_dictionary_t,
					       int, unsigned long,
					       prop_dictionary_t *);
#elif defined(_KERNEL)
struct plistref;

int		prop_dictionary_copyin_ioctl(const struct plistref *,
					     const u_long,
					     prop_dictionary_t *);
int		prop_dictionary_copyout_ioctl(struct plistref *,
					      const u_long,
					      prop_dictionary_t);
#endif
#endif /* __NetBSD__ */
__END_DECLS

#endif /* _PROPLIB_PROP_DICTIONARY_H_ */
