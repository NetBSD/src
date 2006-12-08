/* $NetBSD: kern_fileassoc.c,v 1.13 2006/12/08 13:23:22 yamt Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Elad Efrat.
 * 4. The name of the author may not be used to endorse or promote products
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
__KERNEL_RCSID(0, "$NetBSD: kern_fileassoc.c,v 1.13 2006/12/08 13:23:22 yamt Exp $");

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
#include <sys/hash.h>
#include <sys/fstypes.h>

/* Max. number of hooks. */
#ifndef FILEASSOC_NHOOKS
#define	FILEASSOC_NHOOKS	4
#endif /* !FILEASSOC_NHOOKS */

static struct fileassoc_hash_entry *
fileassoc_file_lookup(struct vnode *, fhandle_t *);
static struct fileassoc_hash_entry *
fileassoc_file_add(struct vnode *, fhandle_t *);

/*
 * Hook entry.
 * Includes the hook name for identification and private hook clear callback.
 */
struct fileassoc_hook {
	const char *hook_name;			/* Hook name. */
	fileassoc_cleanup_cb_t hook_cleanup_cb;	/* Hook clear callback. */
};

/* An entry in the per-device hash table. */
struct fileassoc_hash_entry {
	fhandle_t *handle;				/* File handle */
	void *hooks[FILEASSOC_NHOOKS];			/* Hooks. */
	LIST_ENTRY(fileassoc_hash_entry) entries;	/* List pointer. */
};

LIST_HEAD(fileassoc_hashhead, fileassoc_hash_entry);

struct fileassoc_table {
	struct fileassoc_hashhead *hash_tbl;
	size_t hash_size;				/* Number of slots. */
	struct mount *tbl_mntpt;
	u_long hash_mask;
	void *tables[FILEASSOC_NHOOKS];
	LIST_ENTRY(fileassoc_table) hash_list;		/* List pointer. */
};

struct fileassoc_hook fileassoc_hooks[FILEASSOC_NHOOKS];
int fileassoc_nhooks;

/* Global list of hash tables, one per device. */
LIST_HEAD(, fileassoc_table) fileassoc_tables;

/*
 * Hashing function: Takes a number modulus the mask to give back an
 * index into the hash table.
 */
#define FILEASSOC_HASH(tbl, handle)	\
	(hash32_buf((handle), FHANDLE_SIZE(handle), HASH32_BUF_INIT) \
	 & ((tbl)->hash_mask))

/*
 * Initialize the fileassoc subsystem.
 */
void
fileassoc_init(void)
{
	memset(fileassoc_hooks, 0, sizeof(fileassoc_hooks));
	fileassoc_nhooks = 0;
}

/*
 * Register a new hook.
 */
fileassoc_t
fileassoc_register(const char *name, fileassoc_cleanup_cb_t cleanup_cb)
{
	int i;

	if (fileassoc_nhooks >= FILEASSOC_NHOOKS)
		return (-1);

	for (i = 0; i < FILEASSOC_NHOOKS; i++)
		if (fileassoc_hooks[i].hook_name == NULL)
			break;

	fileassoc_hooks[i].hook_name = name;
	fileassoc_hooks[i].hook_cleanup_cb = cleanup_cb;

	fileassoc_nhooks++;

	return (i);
}

/*
 * Deregister a hook.
 */
int
fileassoc_deregister(fileassoc_t id)
{
	if (id < 0 || id >= FILEASSOC_NHOOKS)
		return (EINVAL);

	fileassoc_hooks[id].hook_name = NULL;
	fileassoc_hooks[id].hook_cleanup_cb = NULL;

	fileassoc_nhooks--;

	return (0);
}

/*
 * Get the hash table for the specified device.
 */
static struct fileassoc_table *
fileassoc_table_lookup(struct mount *mp)
{
	struct fileassoc_table *tbl;

	LIST_FOREACH(tbl, &fileassoc_tables, hash_list) {
		if (tbl->tbl_mntpt == mp)
			return (tbl);
	}

	return (NULL);
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

	if (hint == NULL) {
		error = vfs_composefh_alloc(vp, &th);
		if (error)
			return (NULL);
	} else
		th = hint;

	tbl = fileassoc_table_lookup(vp->v_mount);
	if (tbl == NULL) {
		if (hint == NULL)
			vfs_composefh_free(th);

		return (NULL);
	}

	indx = FILEASSOC_HASH(tbl, th);
	tble = &(tbl->hash_tbl[indx]);

	LIST_FOREACH(e, tble, entries) {
		if ((e != NULL) &&
		    ((FHANDLE_FILEID(e->handle)->fid_len ==
		     FHANDLE_FILEID(th)->fid_len)) &&
		    (memcmp(FHANDLE_FILEID(e->handle), FHANDLE_FILEID(th),
			   (FHANDLE_FILEID(th))->fid_len) == 0)) {
			if (hint == NULL)
				vfs_composefh_free(th);

			return (e);
		}
	}

	if (hint == NULL)
		vfs_composefh_free(th);

	return (NULL);
}

/*
 * Return hook data associated with a vnode.
 */
void *
fileassoc_lookup(struct vnode *vp, fileassoc_t id)
{
        struct fileassoc_hash_entry *mhe;

        mhe = fileassoc_file_lookup(vp, NULL);
        if (mhe == NULL)
                return (NULL);

        return (mhe->hooks[id]);
}

/*
 * Create a new fileassoc table.
 */
int
fileassoc_table_add(struct mount *mp, size_t size)
{
	struct fileassoc_table *tbl;

	/* Check for existing table for device. */
	if (fileassoc_table_lookup(mp) != NULL)
		return (EEXIST);

	/* Allocate and initialize a Veriexec hash table. */
	tbl = malloc(sizeof(*tbl), M_TEMP, M_WAITOK | M_ZERO);
	tbl->hash_size = size;
	tbl->tbl_mntpt = mp;
	tbl->hash_tbl = hashinit(size, HASH_LIST, M_TEMP,
				 M_WAITOK | M_ZERO, &tbl->hash_mask);

	LIST_INSERT_HEAD(&fileassoc_tables, tbl, hash_list);

	return (0);
}

/*
 * Delete a table.
 */
int
fileassoc_table_delete(struct mount *mp)
{
	struct fileassoc_table *tbl;
	struct fileassoc_hashhead *hh;
	u_long i;
	int j;

	tbl = fileassoc_table_lookup(mp);
	if (tbl == NULL)
		return (EEXIST);

	/* Remove all entries from the table and lists */
	hh = tbl->hash_tbl;
	for (i = 0; i < tbl->hash_size; i++) {
		struct fileassoc_hash_entry *mhe;

		while (LIST_FIRST(&hh[i]) != NULL) {
			mhe = LIST_FIRST(&hh[i]);
			LIST_REMOVE(mhe, entries);

			for (j = 0; j < fileassoc_nhooks; j++)
				if (fileassoc_hooks[j].hook_cleanup_cb != NULL)
					(fileassoc_hooks[j].hook_cleanup_cb)
					    (mhe->hooks[j],
					    FILEASSOC_CLEANUP_FILE);

			vfs_composefh_free(mhe->handle);
			free(mhe, M_TEMP);
		}
	}

	for (j = 0; j < fileassoc_nhooks; j++)
		if (fileassoc_hooks[j].hook_cleanup_cb != NULL)
			(fileassoc_hooks[j].hook_cleanup_cb)(tbl->tables[j],
			    FILEASSOC_CLEANUP_TABLE);

	/* Remove hash table and sysctl node */
	hashdone(tbl->hash_tbl, M_TEMP);
	LIST_REMOVE(tbl, hash_list);

	return (0);
}

/*
 * Run a callback for each hook entry in a table.
 */
int
fileassoc_table_run(struct mount *mp, fileassoc_t id, fileassoc_cb_t cb)
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
			if (mhe->hooks[id] != NULL)
				cb(mhe->hooks[id]);
		}
	}

	return (0);
}

/*
 * Clear a table for a given hook.
 */
int
fileassoc_table_clear(struct mount *mp, fileassoc_t id)
{
	struct fileassoc_table *tbl;
	struct fileassoc_hashhead *hh;
	fileassoc_cleanup_cb_t cleanup_cb;
	u_long i;

	tbl = fileassoc_table_lookup(mp);
	if (tbl == NULL)
		return (EEXIST);

	cleanup_cb = fileassoc_hooks[id].hook_cleanup_cb;

	hh = tbl->hash_tbl;
	for (i = 0; i < tbl->hash_size; i++) {
		struct fileassoc_hash_entry *mhe;

		LIST_FOREACH(mhe, &hh[i], entries) {
			if ((mhe->hooks[id] != NULL) && cleanup_cb != NULL)
				cleanup_cb(mhe->hooks[id],
				    FILEASSOC_CLEANUP_FILE);

			mhe->hooks[id] = NULL;
		}
	}

	if ((tbl->tables[id] != NULL) && cleanup_cb != NULL)
		cleanup_cb(tbl->tables[id], FILEASSOC_CLEANUP_TABLE);

	tbl->tables[id] = NULL;

	return (0);
}

/*
 * Add hook-specific data on a fileassoc table.
 */
int
fileassoc_tabledata_add(struct mount *mp, fileassoc_t id, void *data)
{
	struct fileassoc_table *tbl;

	tbl = fileassoc_table_lookup(mp);
	if (tbl == NULL)
		return (EFAULT);

	tbl->tables[id] = data;

	return (0);
}

/*
 * Clear hook-specific data on a fileassoc table.
 */
int
fileassoc_tabledata_clear(struct mount *mp, fileassoc_t id)
{
	struct fileassoc_table *tbl;

	tbl = fileassoc_table_lookup(mp);
	if (tbl == NULL)
		return (EFAULT);

	tbl->tables[id] = NULL;

	return (0);
}

/*
 * Retrieve hook-specific data from a fileassoc table.
 */
void *
fileassoc_tabledata_lookup(struct mount *mp, fileassoc_t id)
{
	struct fileassoc_table *tbl;

	tbl = fileassoc_table_lookup(mp);
	if (tbl == NULL)
		return (NULL);

	return (tbl->tables[id]);
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
		if (hint == NULL)
			vfs_composefh_free(th);

		return (NULL);
	}

	indx = FILEASSOC_HASH(tbl, th);
	vhh = &(tbl->hash_tbl[indx]);

	e = malloc(sizeof(*e), M_TEMP, M_WAITOK | M_ZERO);
	e->handle = th;
	LIST_INSERT_HEAD(vhh, e, entries);

	return (e);
}

/*
 * Delete a file entry from a table.
 */
int
fileassoc_file_delete(struct vnode *vp)
{
	struct fileassoc_hash_entry *mhe;
	int i;

	mhe = fileassoc_file_lookup(vp, NULL);
	if (mhe == NULL)
		return (ENOENT);

	LIST_REMOVE(mhe, entries);

	for (i = 0; i < fileassoc_nhooks; i++)
		if (fileassoc_hooks[i].hook_cleanup_cb != NULL)
			(fileassoc_hooks[i].hook_cleanup_cb)(mhe->hooks[i],
			    FILEASSOC_CLEANUP_FILE);

	free(mhe, M_TEMP);

	return (0);
}

/*
 * Add a hook to a vnode.
 */
int
fileassoc_add(struct vnode *vp, fileassoc_t id, void *data)
{
	struct fileassoc_hash_entry *e;

	e = fileassoc_file_lookup(vp, NULL);
	if (e == NULL) {
		e = fileassoc_file_add(vp, NULL);
		if (e == NULL)
			return (ENOTDIR);
	}

	if (e->hooks[id] != NULL)
		return (EEXIST);

	e->hooks[id] = data;

	return (0);
}

/*
 * Clear a hook from a vnode.
 */
int
fileassoc_clear(struct vnode *vp, fileassoc_t id)
{
	struct fileassoc_hash_entry *mhe;
	fileassoc_cleanup_cb_t cleanup_cb;

	mhe = fileassoc_file_lookup(vp, NULL);
	if (mhe == NULL)
		return (ENOENT);

	cleanup_cb = fileassoc_hooks[id].hook_cleanup_cb;
	if ((mhe->hooks[id] != NULL) && cleanup_cb != NULL)
		cleanup_cb(mhe->hooks[id], FILEASSOC_CLEANUP_FILE);

	mhe->hooks[id] = NULL;

	return (0);
}
