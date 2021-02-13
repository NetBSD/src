/*	$NetBSD: returns.h,v 1.6 2021/02/13 08:14:46 rillig Exp $	*/

/*-
 * Copyright 2009 Brett Lymn <blymn@NetBSD.org>
 * Copyright 2021 Roland Illig <rillig@NetBSD.org>
 *
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef CTF_RETURNS_H
#define CTF_RETURNS_H

typedef enum {
	data_number = 1,
	data_static,
	data_string,
	data_byte,
	data_cchar,
	data_wchar,
	data_err,
	data_ok,
	data_null,
	data_nonnull,
	data_var,
	data_ref,
	data_count,
	data_slave_error
} data_enum_t;

typedef struct {
	data_enum_t	data_type;
	void		*data_value;	/* used if data_type is data_num
					   or data_byte or data_string */
	size_t		data_len;	/* number of bytes in return_value iff
					   return_type is data_byte */
	int		data_index;	/* index into var array for return
					   if data_type is data_var */
} ct_data_t;

typedef struct {
	size_t	count;
	size_t	allocated;
	size_t	readp;
	char	*data;
} saved_data_t;

#endif
