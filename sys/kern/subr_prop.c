/*	$NetBSD: subr_prop.c,v 1.3.2.2 2001/10/11 00:02:32 fvdl Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/sysctl.h>
#include <sys/properties.h>

#ifdef DEBUG

int propdebug = 0;
#define	DPRINTF(v, t)	if (propdebug) printf t
#else
#define	DPRINTF(v, t)
#endif

/* 
 * Kernel properties database implementation.
 *
 * While this could theoretically be flat, lookups
 * are always done in the following order:
 *
 *	database, object, name
 *
 * So we'll lay out the structures to make this efficient.
 *
 */
#define	KDB_SIZE	32	/* Initial hash table size */
#define	KDB_MAXLEN	6	/* Max acceptable bucket length */
#define	KDB_STEP	2	/* Increment size for hash table */
#define	KDB_HASH(v, s)	((((v)>>16)^((v)>>8))&((s)-1))

typedef LIST_HEAD(kobj_head, kdbobj) kobj_bucket_t;

static LIST_HEAD(propdb_list, propdb) propdbs =
	LIST_HEAD_INITIALIZER(propdbs);

struct propdb {
	LIST_ENTRY(propdb)	kd_link;
	char			kd_name[MAX_KDBNAME];
	size_t			kd_size;

	/* Hash table of kdbobj structs */
	kobj_bucket_t		*kd_obj;
	int			kd_longest;	/* Keep track of collisions */
};

struct kdbobj {
	LIST_ENTRY(kdbobj)	ko_link;
	opaque_t		ko_object;
	/* 
	 * There should only be a dozen props for each object,
	 * so we can keep them in a list.
	 */
	LIST_HEAD(kprops, kdbprop) ko_props;
};

struct kdbprop {
	LIST_ENTRY(kdbprop)	kp_link;
	const char		*kp_name;
	const char		*kp_val;
	int			kp_len;
	int			kp_type;
};


static struct kdbprop *allocprop(const char *name, size_t len, int wait);
static void kdb_rehash(struct propdb *db);
static struct kdbobj *kdbobj_find(propdb_t db, opaque_t object, 
	int create, int wait);
static int prop_insert(struct kdbobj *obj, const char *name, void *val, 
	size_t len, int type, int wait);

/* 
 * Allocate a prop structure large enough to hold
 * `name' and `len' bytes of data.  For PROP_CONST
 * pass in a `len' of 0.
 */
static struct kdbprop *
allocprop(const char *name, size_t len, int wait) 
{
	struct kdbprop *kp;
	char *np, *vp;
	size_t dsize, nsize;

	dsize = ALIGN(len);
	nsize = ALIGN(strlen(name));

	DPRINTF(x, ("allocprop: allocating %lu bytes for %s %s\n",
		(unsigned long)(sizeof(struct kdbprop) + dsize + nsize), name, 
		wait ? "can wait" : "can't wait"));

	kp = (struct kdbprop *)malloc(sizeof(struct kdbprop) + dsize + nsize,
		M_PROP,	wait ? M_WAITOK : M_NOWAIT);

	DPRINTF(x, ("allocprop: got %p for prop\n", kp));

	if (kp) {
		/* Install name and init pointers */
		vp = (char *)&kp[1];
		kp->kp_val = (const char *)vp;
		np = vp + dsize;
		strcpy(np, name);
		kp->kp_name = (const char *)np;
		kp->kp_len = len;
	}
	return (kp);
}


/*
 * If the database hash chains grow too long try to resize
 * the hash table.  Failure is not catastrophic.
 */
static void
kdb_rehash(struct propdb *db)
{
	struct kdbobj *obj;
	kobj_bucket_t *new, *old = db->kd_obj;
	long hash;
	size_t newsize = (db->kd_size << KDB_STEP);
	int i, s;

	new = (kobj_bucket_t *)malloc(sizeof(kobj_bucket_t) * newsize,
		M_PROP,	M_NOWAIT);
	if (!new) return;
	s = splvm();
	for (i=0; i<newsize; i++)
		LIST_INIT(&new[i]);

	/* Now pop an object from the old table and insert it in the new one. */
	for (i=0; i<db->kd_size; i++) {
		while ((obj = LIST_FIRST(&old[i]))) {
			LIST_REMOVE(obj, ko_link);
			hash = (long)obj->ko_object;
			hash = KDB_HASH(hash, db->kd_size);
			LIST_INSERT_HEAD(&new[hash], obj, ko_link);
		}
	}
	db->kd_size = newsize;
	db->kd_obj = new;
	splx(s);
	free(old, M_PROP);
}

/*
 * For propdb structures we use a simple power-of-2
 * hash.
 */

propdb_t 
propdb_create(const char *name)
{
	struct propdb *db;
	int i;

	db = (struct propdb *)malloc(sizeof(struct propdb),
		M_PROP,	M_WAITOK);

	strncpy(db->kd_name, name, 32);

	/* Initialize the hash table. */
	db->kd_size = KDB_SIZE;
	db->kd_longest = 0;
	db->kd_obj = (kobj_bucket_t *)malloc(sizeof(kobj_bucket_t) * 
		db->kd_size, M_PROP, M_WAITOK);
	for (i = 0; i < db->kd_size; i++)
		LIST_INIT(&db->kd_obj[i]);
	LIST_INSERT_HEAD(&propdbs, db, kd_link);
	return (db);
}

void 
propdb_destroy(propdb_t db)
{
	struct kdbobj *obj;
	struct kdbprop *prop;
	int i;

#ifdef DIAGNOSTIC
	struct propdb *p;

	/* Make sure we have a handle to a valid database */
	LIST_FOREACH(p, &propdbs, kd_link) {
		if (p == db) break;
	}
	if (p == NULL) panic("propdb_destroy: invalid database\n");
#endif
	LIST_REMOVE(db, kd_link);

	/* Empty out each hash bucket */
	for (i = 0; i < db->kd_size; i++) {
		while ((obj = LIST_FIRST(&db->kd_obj[i]))) {
			LIST_REMOVE(obj, ko_link);
			while ((prop = LIST_FIRST(&obj->ko_props))) {
				LIST_REMOVE(prop, kp_link);
				free(prop, M_PROP);
			}
			free(obj, M_PROP);
		}
	}
	free(db->kd_obj, M_PROP);
	free(db, M_PROP);
}

/*
 * Find an object in the database and possibly create it too.
 */
static struct kdbobj *
kdbobj_find(propdb_t db, opaque_t object, int create, int wait)
{
	struct kdbobj *obj;
	long hash = (long)object;
	int i;

	/* Find our object */
	hash = KDB_HASH(hash, db->kd_size);
	i=0;
	LIST_FOREACH(obj, &db->kd_obj[hash], ko_link) {
		i++;	/* Measure chain depth */
		if (obj->ko_object == object)
			break;
	}
	if (create && (obj == NULL)) {
		/* Need a new object. */
		obj = (struct kdbobj *)malloc(sizeof(struct kdbobj),
			M_PROP,	wait ? M_WAITOK : M_NOWAIT);

		if (!obj) {
			return (obj);
		}

		/* Handle hash table growth */
		if (++i > db->kd_longest) 
			db->kd_longest = i;
		if (db->kd_longest > KDB_MAXLEN) {
			/* Increase the size of our hash table */
			kdb_rehash(db);
		}

		/* Initialize object */
		obj->ko_object = object;
		LIST_INIT(&obj->ko_props);
		LIST_INSERT_HEAD(&db->kd_obj[hash], obj, ko_link);
	}
	return (obj);
}

/*
 * Internal property insertion routine. 
 */
static int
prop_insert(struct kdbobj *obj, const char *name, void *val, size_t len,
	int type, int wait)
{
	struct kdbprop *prop = NULL, *oprop;

	/* Does the prop exist already? */
	LIST_FOREACH(oprop, &obj->ko_props, kp_link) {
		if (strcmp(oprop->kp_name, name) == 0)
			break;
	}
	if (oprop) {
		/* Can is it big enough? */
		if ((type & PROP_CONST) ||
			((ALIGN(len) < ALIGN(oprop->kp_len)) &&
				(oprop->kp_type & PROP_CONST) == 0)) {
			/* We can reuse it */
			prop = oprop;
		} 
	}
	if (!prop) {
		/* Allocate a new prop */
		if (type & PROP_CONST)
			prop = allocprop(name, 0, wait);
		else
			prop = allocprop(name, len, wait);
		if (!prop) return (wait ? ENOMEM : EAGAIN);
	}
	/* Set the values */
	if (type & PROP_CONST) {
		prop->kp_val = val;
	} else {
		char *dest = (char *)prop->kp_val;
		memcpy(dest, val, len);
	}
	prop->kp_len = len;
	prop->kp_type = type;

	/* Now clean up if necessary */
	if (prop != oprop) {
		LIST_INSERT_HEAD(&obj->ko_props, prop, kp_link);
		if (oprop) {
			LIST_REMOVE(oprop, kp_link);
			free(oprop, M_PROP);
		}
	}
	return (0);
}

int 
prop_set(propdb_t db, opaque_t object, const char *name,
	void *val, size_t len, int type, int wait)
{
	struct kdbobj *obj;
	struct kdbprop *prop = NULL, *oprop;
	int s;

	DPRINTF(x, ("prop_set: %p, %p, %s, %p, %lx, %x, %d\n", db, object, 
		name ? name : "NULL", val, (unsigned long)len, type, wait));

	/* Find our object */
	s = splvm();
	obj = kdbobj_find(db, object, 1, wait);
	if (!obj) {
		splx(s);
		return (wait ? ENOMEM : EAGAIN);
	}

#if 1
	{
		int rv;

oprop = prop; /* XXXX -- use vars to make gcc happy. */
		rv = prop_insert(obj, name, val, len, type, wait);
		splx(s);
		return (rv);
	}
#else
	/* Does the prop exist already? */
	LIST_FOREACH(oprop, &obj->ko_props, kp_link) {
		if (strcmp(oprop->kp_name, name) == 0)
			break;
	}
	if (oprop) {
		/* Can is it big enough? */
		if ((type & PROP_CONST) ||
			((ALIGN(len) < ALIGN(oprop->kp_len)) &&
				(oprop->kp_type & PROP_CONST) == 0)) {
			/* We can reuse it */
			prop = oprop;
		} 
	}
	if (!prop) {
		/* Allocate a new prop */
		if (type & PROP_CONST)
			prop = allocprop(name, 0, wait);
		else
			prop = allocprop(name, len, wait);
		if (!prop) return (wait ? ENOMEM : EAGAIN);
	}
	/* Set the values */
	if (type & PROP_CONST) {
		prop->kp_val = val;
	} else {
		char *dest = (char *)prop->kp_val;
		memcpy(dest, val, len);
	}
	prop->kp_len = len;
	prop->kp_type = type;

	/* Now clean up if necessary */
	if (prop != oprop) {
		LIST_INSERT_HEAD(&obj->ko_props, prop, kp_link);
		if (oprop) {
			LIST_REMOVE(oprop, kp_link);
			free(oprop, M_PROP);
		}
	}
	splx(s);
	return (0);
#endif
}

size_t 
prop_get(propdb_t db, opaque_t object, const char *name, void *val, 
	size_t len, int *type)
{
	struct kdbobj *obj;
	struct kdbprop *prop = NULL;
	int s;

	DPRINTF(x, ("prop_get: %p, %p, %s, %p, %lx, %p\n", db, object, 
		name ? name : "NULL", val, (unsigned long)len, type));

	/* Find our object */
	s = splvm();
	obj = kdbobj_find(db, object, 0, 0);
	if (!obj) {
		splx(s);
		return (-1);
	}

	/* find our prop */
	LIST_FOREACH(prop, &obj->ko_props, kp_link) {
		if (strcmp(prop->kp_name, name) == 0)
			break;
	}
	if (!prop) {
		splx(s);
		return (-1);
	}

	/* Copy out our prop */
	len = min(len, prop->kp_len);
	if (val && len) {
		memcpy(val, prop->kp_val, len);
	}
	if (type) 
		*type = prop->kp_type;
	splx(s);
	return (prop->kp_len);
}

/*
 * Return the total number of objects in the database and as
 * many as fit in the buffer.
 */
size_t
prop_objs(propdb_t db, opaque_t *objects, size_t len) 
{
	struct kdbobj *obj;
	int i, j, s, nelem = (len / sizeof(opaque_t));

	DPRINTF(x, ("prop_objs: %p, %p, %lx\n", db, objects,
	    (unsigned long)len));

	s = splvm();
	for (i=0, j=0; i < db->kd_size; i++) {
		LIST_FOREACH(obj, &db->kd_obj[i], ko_link) {
			if (objects && j<nelem)
				objects[j] = obj->ko_object;
			j++;
		}
	}
	splx(s);
	return (j * sizeof(opaque_t));
}

/*
 * Return the total number of property names associated with an object
 * and as many as fit in the buffer.
 */
size_t
prop_list(propdb_t db, opaque_t object, char *names, size_t len) 
{
	struct kdbobj *obj;
	struct kdbprop *prop = NULL;
	size_t total_len = 0;
	int s, i = 0;

	DPRINTF(x, ("prop_list: %p, %p, %p, %lx\n", 
		db, object, names, (unsigned long)len));

	/* Find our source object */
	s = splvm();
	obj = kdbobj_find(db, object, 0, 0);
	if (obj == NULL) {
		splx(s);
		return (0);
	}

	LIST_FOREACH(prop, &obj->ko_props, kp_link) {
		i = strlen(prop->kp_name) + 1;
		total_len += i;
		if (total_len < len) {
			strcpy(names, prop->kp_name);
			names += i;
			/* Add an extra NUL */
			names[i+1] = 0;
		}
	}
	splx(s);
	return (total_len);
}

int 
prop_delete(propdb_t db, opaque_t object, const char *name)
{
	struct kdbobj *obj;
	struct kdbprop *prop = NULL;
	int s, i = 0;

	DPRINTF(x, ("prop_delete: %p, %p, %s\n", db, object, 
		name ? name : "NULL"));

	/* Find our object */
	s = splvm();
	obj = kdbobj_find(db, object, 0, 0);
	if (obj == NULL) {
		splx(s);
		return (0);
	}

	if (name) {
		/* Find our prop */
		LIST_FOREACH(prop, &obj->ko_props, kp_link) {
			if (strcmp(prop->kp_name, name) == 0)
				break;
		}
		if (!prop) {
			splx(s);
			return (0);
		}
		LIST_REMOVE(prop, kp_link);
		free(prop, M_PROP);
		i++;
	} else {
		while ((prop = LIST_FIRST(&obj->ko_props))) {
			LIST_REMOVE(prop, kp_link);
			free(prop, M_PROP);
			i++;
		}
	}
	if (LIST_EMPTY(&obj->ko_props)) {
		/* Free up the empty container. */
		LIST_REMOVE(obj, ko_link);
		free(obj, M_PROP);
	}
	splx(s);
	return (i);
}

int 
prop_copy(propdb_t db, opaque_t source, opaque_t dest, int wait)
{
	struct kdbobj *nobj, *oobj;
	struct kdbprop *prop, *oprop, *srcp;
	int s;

	DPRINTF(x, ("prop_copy: %p, %p, %p, %d\n", db, source, dest, wait));

	/* Find our source object */
	s = splvm();
	oobj = kdbobj_find(db, source, 0, wait);
	if (oobj == NULL) {
		splx(s);
		return (EINVAL);
	}

	/* Find our dest object */
	nobj = kdbobj_find(db, dest, 1, wait);
	if (!nobj) {
		splx(s);
		return (wait ? ENOMEM : EAGAIN);
	}

	/* Copy these properties over now */
	LIST_FOREACH(srcp, &oobj->ko_props, kp_link) {

		DPRINTF(x, ("prop_copy: copying prop %s\n", 
			srcp->kp_name));

#if 1
		{
			int rv;

oprop = prop; /* XXXX -- use vars to make gcc happy. */
			rv = prop_insert(nobj, srcp->kp_name, 
				(void *)srcp->kp_val, srcp->kp_len,
				srcp->kp_type, wait);
			if (rv) {
				/* Error of some sort */
				splx(s);
				return (rv);
			}
		}
#else
		/* Does the prop exist already? */
		prop = NULL;
		LIST_FOREACH(oprop, &nobj->ko_props, kp_link) {
			if (strcmp(oprop->kp_name, srcp->kp_name) == 0)
				break;
		}
		if (oprop) {

			DPRINTF(x, ("prop_copy: found old prop %p\n",
				oprop));

			/* Can is it big enough? */
			if ((srcp->kp_type & PROP_CONST) ||
				((ALIGN(srcp->kp_len) < ALIGN(oprop->kp_len)) &&
					(oprop->kp_type & PROP_CONST) == 0)) {
				/* We can reuse it */
				prop = oprop;
				DPRINTF(x, ("prop_copy: using old prop\n"));
			}
		}
		if (!prop) {
			/* Allocate a new prop */
			if (srcp->kp_type & PROP_CONST)
				prop = allocprop(srcp->kp_name, 0, wait);
			else
				prop = allocprop(srcp->kp_name, 
					srcp->kp_len, wait);
			if (!prop) {
				splx(s);
				return (wait ? ENOMEM : EAGAIN);
			}
		}
		/* Set the values */
		if (srcp->kp_type & PROP_CONST) {
			prop->kp_val = srcp->kp_val;
		} else {
			char *dest = (char *)prop->kp_val;
			memcpy(dest, srcp->kp_val, srcp->kp_len);
		}
		prop->kp_len = srcp->kp_len;
		prop->kp_type = srcp->kp_type;

		/* Now clean up if necessary */
		if (prop != oprop) {

			DPRINTF(x, ("prop_copy: inserting prop %p\n", prop));
			LIST_INSERT_HEAD(&nobj->ko_props, prop, kp_link);
			if (oprop) {
				DPRINTF(x, ("prop_copy: removing prop %p\n", 
					oprop));
				LIST_REMOVE(oprop, kp_link);
				free(oprop, M_PROP);
			}
		}
#endif
	}
	DPRINTF(x, ("prop_copy: done\n"));
	splx(s);
	return (0);

}

