/*	$NetBSD: properties.h,v 1.2.2.2 2001/10/11 00:02:35 fvdl Exp $	*/

/*  
 * Copyright (c) 2001 Eduardo Horvath.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Eduardo Horvath.
 * 4. The name of the author may not be used to endorse or promote products
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

#ifndef _SYS_PROPERTIES_H_
#define _SYS_PROPERTIES_H_

typedef void *opaque_t;		/* Value large enough to hold a pointer */

struct propdb;
typedef struct propdb *propdb_t;

#define	MAX_KDBNAME		32

#define	PROP_INT	0x10000000
#define	PROP_STRING	0x20000000
#define	PROP_AGGREGATE	0x30000000
#define	PROP_TYPE(x)	((x)&0x30000000)

#define	PROP_ARRAY	0x40000000
#define	PROP_CONST	0x80000000
#define	PROP_ELSZ(x)	0x0fffffff

propdb_t propdb_create(const char *name);
void propdb_destroy(propdb_t db);

int prop_set(propdb_t db, opaque_t object, const char *name,
	void *val, size_t len, int type, int wait);
size_t prop_objs(propdb_t db, opaque_t *objects, size_t len);
size_t prop_list(propdb_t db, opaque_t object, char *names,
	size_t len);
size_t prop_get(propdb_t db, opaque_t object,	const char *name,
	void *val, size_t len, int *type);
int prop_delete(propdb_t db, opaque_t object, const char *name);
int prop_copy(propdb_t db, opaque_t source, opaque_t dest,
	int wait);

#endif
