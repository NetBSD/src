/*	$NetBSD: bdb.h,v 1.1.1.1 2004/05/17 23:43:51 christos Exp $	*/

/*
 * Copyright (C) 2002  Nuno M. Rodrigues.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND NUNO M. RODRIGUES
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * INTERNET SOFTWARE CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* Id: bdb.h,v 1.1.4.1 2002/07/02 04:45:36 marka Exp */

#ifndef BDB_H
#define BDB_H	1

#include <isc/types.h>

/*
 * Prototypes.
 */
isc_result_t	bdb_init(void);
void		bdb_clear(void);

#endif /* BDB_H */
