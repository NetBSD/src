/*	$NetBSD: properties.h,v 1.1 2001/10/04 18:56:06 eeh Exp $	*/

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

int sysctl_propdb(int *name, u_int namelen, void *oldp, size_t *oldlenp,
	void *newp, size_t newlen);


/*
 * KERN_DB subtypes
 */

#define	KERN_DB_ALL		 0	/* all databases */
#define	KERN_DB_OBJ_ALL		 1	/* all objects in a database */
#define	KERN_DB_PROP_ALL	 2	/* all properties in an object */
#define	KERN_DB_PROP_GET	 3	/* get a single property */
#define	KERN_DB_PROP_SET	 4	/* set a single property */
#define	KERN_DB_PROP_DELETE	 5	/* delete a single property */

#define	CTL_DB_NAMES { \
	{ 0, 0 }, \
	{ "all", CTLTYPE_STRUCT }, \
	{ "allobj", CTLTYPE_QUAD }, \
	{ "allprops", CTLTYPE_INT }, \
	{ "propget", CTLTYPE_STRUCT }, \
	{ "propset", CTLTYPE_STRUCT }, \
	{ "propdelete", CTLTYPE_STRUCT }, \
}

/* Info available for each database. */
struct kinfo_kdb {
	char		ki_name[MAX_KDBNAME];
	uint64_t	ki_id;
};

/* A single property */
struct kinfo_prop {
	uint32_t	kip_len;	/* total len of this prop */
	uint32_t	kip_type;	/* type of this prop */
	uint32_t	kip_valoff;	/* offset of start of value */
	uint32_t	kip_vallen;	/* length of value */
	char		kip_name[1];	/* name of this property */
};

/* Use these macros to encode database and object information in the MIB. */
#define	KERN_DB_ENCODE_DB(p, m)		do {			\
		(m)[2] = (int)(((uint64_t)(long)(p))>>32);	\
		(m)[3] = (int)(p);				\
	} while (0)

#define	KERN_DB_ENCODE_OBJ(p, o, m)	do {			\
		(m)[2] = (int)(((uint64_t)(long)(p))>>32);	\
		(m)[3] = (int)(p);				\
		(m)[4] = (int)(((uint64_t)(long)(o))>>32);	\
		(m)[5] = (int)(o);				\
	} while (0)
#endif
