/* $NetBSD: kern_fileassoc.c,v 1.30 2008/01/02 11:48:49 ad Exp $ */

/*-
 * Copyright (c) 2006 Elad Efrat <elad@NetBSD.org>
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_fileassoc.c,v 1.30 2008/01/02 11:48:49 ad Exp $");

#include "opt_fileassoc.h"

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/inttypes.h>
#include <sys/errno.h>
#include <sys/fileassoc.h>
#include <sys/specificdata.h>
#include <sys/hash.h>
#include <sys/fstypes.h>
#include <sys/kmem.h>
#include <sys/once.h>

#define	FILEASSOC_INITIAL_TABLESIZE	128

static struct fileassoc_hash_entry *
fileassoc_file_lookup(struct vnode *, fhandle_t *);
static struct fileassoc_hash_entry *
fileassoc_file_add(struct vnode *, fhandle_t *);
static struct fileassoc_table *fileassoc_table_resize(struct fileassoc_table *);

static specificdata_domain_t fileassoc_domain;
static specificdata_key_t fileassoc_mountspecific_key;
static ONCE_DECL(control);

/*
 * Hook entry.
 * Includes the hook name for identification and private hook clear callback.
 */
struct fileassoc {
	LIST_ENTRY(fileassoc) list;
	const char *name;			/* name. */
	fileassoc_cleanup_cb_t cleanup_cb;	/* clear callback. */
	specificdata_key_t key;
};

static LIST_HEAD(, fileassoc) fileassoc_list;

/* An entry in the per-mount hash table. */
struct fileassoc_hash_entry {
	fhandle_t *handle;				/* File handle */
	specificdata_reference data;			/* Hooks. */
	u_int nassocs;					/* # of hooks. */
	LIST_ENTRY(fileassoc_hash_entry) entries;	/* List pointer. */
};

LIST_HEAD(fileassoc_hashhead, fileassoc_hash_entry);

struct fileassoc_table {
	struct fileassoc_hashhead *hash_tbl;
	size_t hash_size;				/* Number of slots. */
	u_long hash_mask;
	size_t hash_used;				/* # of used slots. */
	specificdata_reference data;
};

/*
 * Hashing function: Takes a number modulus the mask to give back an
 * index into the hash table.
 */
#define FILEASSOC_HASH(tbl, handle)	\
	(hash32_buf((handle), FHANDLE_SIZE(handle), HASH32_BUF_INIT) \
	 & ((tbl)->hash_mask))

static void *
file_getdata(struct fileassoc_hash_entry *e, const struct fileassoc *assoc)
{

	return specificdata_getspecific(fileassoc_domain, &e->data,
	    assoc->key);
}

static void
file_setdata(struct fileassoc_hash_entry *e, const struct fileassoc *assoc,
    void *data)
{

	specificdata_setspecific(fileassoc_domain, &e->data, assoc->key,
	    data);
}

static void
file_cleanup(struct fileassoc_hash_entry *e, const struct fileassoc *assoc)
{
	fileassoc_cleanup_cb_t cb;
	void *data;

	cb = assoc->cleanup_cb;
	if (cb == NULL) {
		return;
	}
	data = file_getdata(e, assoc);
	(*cb)(data);
}

static void
file_free(struct fileassoc_hash_entry *e)
{
	struct fileassoc *assoc;

	LIST_REMOVE(e, entries);

	LIST_FOREACH(assoc, &fileassoc_list, list) {
		file_cleanup(e, assoc);
	}
	vfs_composefh_free(e->handle);
	specificdata_fini(fileassoc_domain, &e->data);
	kmem_free(e, sizeof(*e));
}

static void
table_dtor(void *vp)
{
	struct fileassoc_table *tbl = vp;
	struct fileassoc_hashhead *hh;
	u_long i;

	/* Remove all entries from the table and lists */
	hh = tbl->hash_tbl;
	for (i = 0; i < tbl->hash_size; i++) {
		struct fileassoc_hash_entry *mhe;

		while ((mhe = LIST_FIRST(&hh[i])) != NULL) {
			file_free(mhe);
		}
	}

	/* Remove hash table and sysctl node */
	hashdone(tbl->hash_tbl, M_TEMP);
	specificdata_fini(fileassoc_domain, &tbl->data);
	kmem_free(tbl, sizeof(*tbl));
}

/*
 * Initialize the fileassoc subsystem.
 */
static int
fileassoc_init(void)
{
	int error;

	error = mount_specific_key_create(&fileassoc_mountspecific_key,
	    table_dtor);
	if (error) {
		return error;
	}
	fileassoc_domain = specificdata_domain_create();

	return 0;
}

/*
 * Register a new hook.
 */
int
fileassoc_register(const char *name, fileassoc_cleanup_cb_t cleanup_cb,
    fileassoc_t *result)
{
	int error;
	specificdata_key_t key;
	struct fileassoc *assoc;

	error = RUN_ONCE(&control, fileassoc_init);
	if (error) {
		return error;
	}
	error = specificdata_key_create(fileassoc_domain, &key, NULL);
	if (error) {
		return error;
	}
	assoc = kmem_alloc(sizeof(*assoc), KM_SLEEP);
	assoc->name = name;
	assoc->cleanup_cb = cleanup_cb;
	assoc->key = key;
	LIST_INSERT_HEAD(&fileassoc_list, assoc, list);
	*result = assoc;

	return 0;
}

/*
 * Deregister a hook.
 */
int
fileassoc_deregister(fileassoc_t assoc)
{

	LIST_REMOVE(assoc, list);
	specificdata_key_delete(fileassoc_domain, assoc->key);
	kmem_free(assoc, sizeof(*assoc));

	return 0;
}

/*
 * Get the hash table for the specified device.
 */
static struct fileassoc_table *
fileassoc_table_lookup(struct mount *mp)
{
	int error;

	error = RUN_ONCE(&control, fileassoc_init);
	if (error) {
		return NULL;
	}
	return mount_getspecific(mp, fileassoc_mountspecific_key);
}

/*
 * Perform a lookup on a hash table.  If hint is non-zero then use the value
 * of the hint as the identifier instead of performing a lookup for the
 * fileid.
 */
static struct fileassoc_hash_entry *
fileassoc_file_lookup(struct vnode *vp, fhandle_t *hint)
{
	struct fileassoc_table *tbl;
	struct fileassoc_hashhead *tble;
	struct fileassoc_hash_entry *e;
	size_t indx;
	fhandle_t *th;
	int error;

	tbl = fileassoc_table_lookup(vp->v_mount);
	if (tbl == NULL) {
		return NULL;
	}

	if (hint == NULL) {
		error = vfs_composefh_alloc(vp, &th);
		if (error)
			return (NULL);
	} else {
		th = hint;
	}

	indx = FILEASSOC_HASH(tbl, th);
	tble = &(tbl->hash_tbl[indx]);

	LIST_FOREACH(e, tble, entries) {
		if (((FHANDLE_FILEID(e->handle)->fid_len ==
		     FHANDLE_FILEID(th)->fid_len)) &&
		    (memcmp(FHANDLE_FILEID(e->handle), FHANDLE_FILEID(th),
			   (FHANDLE_FILEID(th))->fid_len) == 0)) {
			break;
		}
	}

	if (hint == NULL)
		vfs_composefh_free(th);

	return e;
}

/*
 * Return hook data associated with a vnode.
 */
void *
fileassoc_lookup(struct vnode *vp, fileassoc_t assoc)
{
        struct fileassoc_hash_entry *mhe;

        mhe = fileassoc_file_lookup(vp, NULL);
        if (mhe == NULL)
                return (NULL);

        return file_getdata(mhe, assoc);
}

static struct fileassoc_table *
fileassoc_table_resize(struct fileassoc_table *tbl)
{
	struct fileassoc_table *newtbl;
	struct fileassoc_hashhead *hh;
	u_long i;

	/*
	 * Allocate a new table. Like the condition in fileassoc_file_add(),
	 * this is also temporary -- just double the number of slots.
	 */
	newtbl = kmem_zalloc(sizeof(*newtbl), KM_SLEEP);
	newtbl->hash_size = (tbl->hash_size * 2);
	if (newtbl->hash_size < tbl->hash_size)
		newtbl->hash_size = tbl->hash_size;
	newtbl->hash_tbl = hashinit(newtbl->hash_size, HASH_LIST, M_TEMP,
	    M_WAITOK | M_ZERO, &newtbl->hash_mask);
	newtbl->hash_used = 0;
	specificdata_init(fileassoc_domain, &newtbl->data);

	/* XXX we need to make sure nothing uses fileassoc here! */

	hh = tbl->hash_tbl;
	for (i = 0; i < tbl->hash_size; i++) {
		struct fileassoc_hash_entry *mhe;

		while ((mhe = LIST_FIRST(&hh[i])) != NULL) {
			struct fileassoc_hashhead *vhh;
			size_t indx;

			LIST_REMOVE(mhe, entries);

			indx = FILEASSOC_HASH(newtbl, mhe->handle);
			vhh = &(newtbl->hash_tbl[indx]);

			LIST_INSERT_HEAD(vhh, mhe, entries);

			newtbl->hash_used++;
		}
	}

	if (tbl->hash_used != newtbl->hash_used)
		panic("fileassoc_table_resize: inconsistency detected! "
		    "needed %zu entries, got %zu", tbl->hash_used,
		    newtbl->hash_used);

	hashdone(tbl->hash_tbl, M_TEMP);
	specificdata_fini(fileassoc_domain, &tbl->data);
	kmem_free(tbl, sizeof(*tbl));

	return (newtbl);
}

/*
 * Create a new fileassoc table.
 */
static struct fileassoc_table *
fileassoc_table_add(struct mount *mp)
{
	struct fileassoc_table *tbl;

	/* Check for existing table for device. */
	tbl = fileassoc_table_lookup(mp);
	if (tbl != NULL)
		return (tbl);

	/* Allocate and initialize a table. */
	tbl = kmem_zalloc(sizeof(*tbl), KM_SLEEP);
	tbl->hash_size = FILEASSOC_INITIAL_TABLESIZE;
	tbl->hash_tbl = hashinit(tbl->hash_size, HASH_LIST, M_TEMP,
	    M_WAITOK | M_ZERO, &tbl->hash_mask);
	tbl->hash_used = 0;
	specificdata_init(fileassoc_domain, &tbl->data);

	mount_setspecific(mp, fileassoc_mountspecific_key, tbl);

	return (tbl);
}

/*
 * Delete a table.
 */
int
fileassoc_table_delete(struct mount *mp)
{
	struct fileassoc_table *tbl;

	tbl = fileassoc_table_lookup(mp);
	if (tbl == NULL)
		return (EEXIST);

	mount_setspecific(mp, fileassoc_mountspecific_key, NULL);
	table_dtor(tbl);

	return (0);
}

/*
 * Run a callback for each hook entry in a table.
 */
int
fileassoc_table_run(struct mount *mp, fileassoc_t assoc, fileassoc_cb_t cb,
    void *cookie)
{
	struct fileassoc_table *tbl;
	struct fileassoc_hashhead *hh;
	u_long i;

	tbl = fileassoc_table_lookup(mp);
	if (tbl == NULL)
		return (EEXIST);

	hh = tbl->hash_tbl;
	for (i = 0; i < tbl->hash_size; i++) {
		struct fileassoc_hash_entry *mhe;

		LIST_FOREACH(mhe, &hh[i], entries) {
			void *data;

			data = file_getdata(mhe, assoc);
			if (data != NULL)
				cb(data, cookie);
		}
	}

	return (0);
}

/*
 * Clear a table for a given hook.
 */
int
fileassoc_table_clear(struct mount *mp, fileassoc_t assoc)
{
	struct fileassoc_table *tbl;
	struct fileassoc_hashhead *hh;
	u_long i;

	tbl = fileassoc_table_lookup(mp);
	if (tbl == NULL)
		return (EEXIST);

	hh = tbl->hash_tbl;
	for (i = 0; i < tbl->hash_size; i++) {
		struct fileassoc_hash_entry *mhe;

		LIST_FOREACH(mhe, &hh[i], entries) {
			file_cleanup(mhe, assoc);
			file_setdata(mhe, assoc, NULL);
		}
	}

	return (0);
}

/*
 * Add a file entry to a table.
 */
static struct fileassoc_hash_entry *
fileassoc_file_add(struct vnode *vp, fhandle_t *hint)
{
	struct fileassoc_table *tbl;
	struct fileassoc_hashhead *vhh;
	struct fileassoc_hash_entry *e;
	size_t indx;
	fhandle_t *th;
	int error;

	if (hint == NULL) {
		error = vfs_composefh_alloc(vp, &th);
		if (error)
			return (NULL);
	} else
		th = hint;

	e = fileassoc_file_lookup(vp, th);
	if (e != NULL) {
		if (hint == NULL)
			vfs_composefh_free(th);

		return (e);
	}

	tbl = fileassoc_table_lookup(vp->v_mount);
	if (tbl == NULL) {
		tbl = fileassoc_table_add(vp->v_mount);
	}

	indx = FILEASSOC_HASH(tbl, th);
	vhh = &(tbl->hash_tbl[indx]);

	e = kmem_zalloc(sizeof(*e), KM_SLEEP);
	e->handle = th;
	specificdata_init(fileassoc_domain, &e->data);
	LIST_INSERT_HEAD(vhh, e, entries);

	/*
	 * This decides when we need to resize the table. For now,
	 * resize it whenever we "filled" up the number of slots it
	 * has. That's not really true unless of course we had zero
	 * collisions. Think positive! :)
	 */
	if (++(tbl->hash_used) == tbl->hash_size) { 
		struct fileassoc_table *newtbl;

		newtbl = fileassoc_table_resize(tbl);
		mount_setspecific(vp->v_mount, fileassoc_mountspecific_key,
		    newtbl);
	}

	return (e);
}

/*
 * Delete a file entry from a table.
 */
int
fileassoc_file_delete(struct vnode *vp)
{
	struct fileassoc_table *tbl;
	struct fileassoc_hash_entry *mhe;

	KERNEL_LOCK(1, NULL);

	mhe = fileassoc_file_lookup(vp, NULL);
	if (mhe == NULL) {
		KERNEL_UNLOCK_ONE(NULL);
		return (ENOENT);
	}

	file_free(mhe);

	tbl = fileassoc_table_lookup(vp->v_mount);
	--(tbl->hash_used); /* XXX gc? */

	KERNEL_UNLOCK_ONE(NULL);

	return (0);
}

/*
 * Add a hook to a vnode.
 */
int
fileassoc_add(struct vnode *vp, fileassoc_t assoc, void *data)
{
	struct fileassoc_hash_entry *e;
	void *olddata;

	e = fileassoc_file_lookup(vp, NULL);
	if (e == NULL) {
		e = fileassoc_file_add(vp, NULL);
		if (e == NULL)
			return (ENOTDIR);
	}

	olddata = file_getdata(e, assoc);
	if (olddata != NULL)
		return (EEXIST);

	file_setdata(e, assoc, data);

	e->nassocs++;

	return (0);
}

/*
 * Clear a hook from a vnode.
 */
int
fileassoc_clear(struct vnode *vp, fileassoc_t assoc)
{
	struct fileassoc_hash_entry *mhe;

	mhe = fileassoc_file_lookup(vp, NULL);
	if (mhe == NULL)
		return (ENOENT);

	file_cleanup(mhe, assoc);
	file_setdata(mhe, assoc, NULL);

	--(mhe->nassocs); /* XXX gc? */

	return (0);
}
