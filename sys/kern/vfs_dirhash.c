/* $NetBSD: vfs_dirhash.c,v 1.1 2008/09/27 13:01:07 reinoud Exp $ */

/*
 * Copyright (c) 2008 Reinoud Zandijk
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
 * 
 */


#include <sys/cdefs.h>
#ifndef lint
__KERNEL_RCSID(0, "$NetBSD: vfs_dirhash.c,v 1.1 2008/09/27 13:01:07 reinoud Exp $");
#endif /* not lint */

#if 1
#	define DPRINTF(a) ;
#else
#	define DPRINTF(a) printf(a);
#endif

/* CLEAN UP! */
#include <sys/sysctl.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/conf.h>
#include <sys/kauth.h>
#include <dev/clock_subr.h>

#include <sys/dirhash.h>


static struct sysctllog *sysctl_log;
struct pool dirhash_pool;
struct pool dirhash_entry_pool;

kmutex_t dirhashmutex;
uint32_t maxdirhashsize = DIRHASH_SIZE;
uint32_t dirhashsize    = 0;
TAILQ_HEAD(_dirhash, dirhash) dirhash_queue;


#define CURDIRHASHSIZE_SYSCTLOPT 1
#define MAXDIRHASHSIZE_SYSCTLOPT 2
void
dirhash_init(void)
{
	size_t size;
	uint32_t max_entries;

	/* initialise dirhash queue */
	TAILQ_INIT(&dirhash_queue);

	/* init dirhash pools */
	size = sizeof(struct dirhash);
	pool_init(&dirhash_pool, size, 0, 0, 0,
		"dirhash_pool", NULL, IPL_NONE);

	size = sizeof(struct dirhash_entry);
	pool_init(&dirhash_entry_pool, size, 0, 0, 0,
		"dirhash_entry_pool", NULL, IPL_NONE);

	mutex_init(&dirhashmutex, MUTEX_DEFAULT, IPL_NONE);
	max_entries = maxdirhashsize / size;
	pool_sethiwat(&dirhash_entry_pool, max_entries);
	dirhashsize = 0;

	/* create sysctl knobs and dials */
	sysctl_log = NULL;
#if 0
	sysctl_createv(&sysctl_log, 0, NULL, &node,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);
	sysctl_createv(&sysctl_log, 0, NULL, &node,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "curdirhashsize",
		       SYSCTL_DESCR("Current memory to be used by dirhash"),
		       NULL, 0, &dirhashsize, 0,
		       CTL_VFS, 0, CURDIRHASHSIZE_SYSCTLOPT, CTL_EOL);
	sysctl_createv(&sysctl_log, 0, NULL, &node,
		       CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		       CTLTYPE_INT, "maxdirhashsize",
		       SYSCTL_DESCR("Max memory to be used by dirhash"),
		       NULL, 0, &maxdirhashsize, 0,
		       CTL_VFS, 0, MAXDIRHASHSIZE_SYSCTLOPT, CTL_EOL);
#endif
}


#if 0
void
dirhash_finish(void)
{
	pool_destroy(&dirhash_pool);
	pool_destroy(&dirhash_entry_pool);

	mutex_destroy(&dirhashmutex);

	/* sysctl_teardown(&sysctl_log); */
}
#endif


/*
 * generic dirhash implementation
 */

static uint32_t
dirhash_hash(const char *str, int namelen)
{
	uint32_t hash = 5381;
        int i, c;

	for (i = 0; i < namelen; i++) {
		c = *str++;
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}
        return hash;
}


void
dirhash_purge_entries(struct dirhash *dirh)
{
	struct dirhash_entry *dirh_e;
	uint32_t hashline;

	if (dirh == NULL)
		return;

	if (dirh->size == 0)
		return;

	for (hashline = 0; hashline < DIRHASH_HASHSIZE; hashline++) {
		dirh_e = LIST_FIRST(&dirh->entries[hashline]);
		while (dirh_e) {
			LIST_REMOVE(dirh_e, next);
			pool_put(&dirhash_entry_pool, dirh_e);
			dirh_e = LIST_FIRST(&dirh->entries[hashline]);
		}
	}
	dirh_e = LIST_FIRST(&dirh->free_entries);

	while (dirh_e) {
		LIST_REMOVE(dirh_e, next);
		pool_put(&dirhash_entry_pool, dirh_e);
		dirh_e = LIST_FIRST(&dirh->entries[hashline]);
	}

	dirh->flags &= ~DIRH_COMPLETE;
	dirh->flags |=  DIRH_PURGED;

	dirhashsize -= dirh->size;
	dirh->size = 0;
}


void
dirhash_purge(struct dirhash **dirhp)
{
	struct dirhash *dirh = *dirhp;

	if (dirh == NULL)
		return;

	mutex_enter(&dirhashmutex);

	dirhash_purge_entries(dirh);
	TAILQ_REMOVE(&dirhash_queue, dirh, next);
	pool_put(&dirhash_pool, dirh);

	*dirhp = NULL;

	mutex_exit(&dirhashmutex);
}


void
dirhash_get(struct dirhash **dirhp)
{
	struct dirhash *dirh;
	uint32_t hashline;

	mutex_enter(&dirhashmutex);

	dirh = *dirhp;
	if (*dirhp == NULL) {
		dirh = pool_get(&dirhash_pool, PR_WAITOK);
		*dirhp = dirh;
		memset(dirh, 0, sizeof(struct dirhash));
		for (hashline = 0; hashline < DIRHASH_HASHSIZE; hashline++)
			LIST_INIT(&dirh->entries[hashline]);
		dirh->size   = 0;
		dirh->refcnt = 0;
		dirh->flags  = 0;
	} else {
		TAILQ_REMOVE(&dirhash_queue, dirh, next);
	}

	dirh->refcnt++;
	TAILQ_INSERT_HEAD(&dirhash_queue, dirh, next);

	mutex_exit(&dirhashmutex);
}


void
dirhash_put(struct dirhash *dirh)
{

	mutex_enter(&dirhashmutex);
	dirh->refcnt--;
	mutex_exit(&dirhashmutex);
}


void
dirhash_enter(struct dirhash *dirh,
	struct dirent *dirent, uint64_t offset, uint32_t entry_size, int new)
{
	struct dirhash *del_dirh, *prev_dirh;
	struct dirhash_entry *dirh_e;
	uint32_t hashvalue, hashline;
	int entrysize;

	/* make sure we have a dirhash to work on */
	KASSERT(dirh);
	KASSERT(dirh->refcnt > 0);

	/* are we trying to re-enter an entry? */
	if (!new && (dirh->flags & DIRH_COMPLETE))
		return;

	/* calculate our hash */
	hashvalue = dirhash_hash(dirent->d_name, dirent->d_namlen);
	hashline  = hashvalue & DIRHASH_HASHMASK;

	/* lookup and insert entry if not there yet */
	LIST_FOREACH(dirh_e, &dirh->entries[hashline], next) {
		/* check for hash collision */
		if (dirh_e->hashvalue != hashvalue)
			continue;
		if (dirh_e->offset != offset)
			continue;
		/* got it already */
		KASSERT(dirh_e->d_namlen == dirent->d_namlen);
		KASSERT(dirh_e->entry_size == entry_size);
		return;
	}

	DPRINTF(("dirhash enter %"PRIu64", %d, %d for `%*.*s`\n",
		offset, entry_size, dirent->d_namlen,
		dirent->d_namlen, dirent->d_namlen, dirent->d_name));

	/* check if entry is in free space list */
	LIST_FOREACH(dirh_e, &dirh->free_entries, next) {
		if (dirh_e->offset == offset) {
			DPRINTF(("\tremoving free entry\n"));
			LIST_REMOVE(dirh_e, next);
			break;
		}
	}

	/* ensure we are not passing the dirhash limit */
	entrysize = sizeof(struct dirhash_entry);
	if (dirhashsize + entrysize > maxdirhashsize) {
		del_dirh = TAILQ_LAST(&dirhash_queue, _dirhash);
		KASSERT(del_dirh);
		while (dirhashsize + entrysize > maxdirhashsize) {
			/* no use trying to delete myself */
			if (del_dirh == dirh)
				break;
			prev_dirh = TAILQ_PREV(del_dirh, _dirhash, next);
			if (del_dirh->refcnt == 0)
				dirhash_purge_entries(del_dirh);
			del_dirh = prev_dirh;
		}
	}

	/* add to the hashline */
	dirh_e = pool_get(&dirhash_entry_pool, PR_WAITOK);
	memset(dirh_e, 0, sizeof(struct dirhash_entry));

	dirh_e->hashvalue = hashvalue;
	dirh_e->offset    = offset;
	dirh_e->d_namlen  = dirent->d_namlen;
	dirh_e->entry_size  = entry_size;

	dirh->size  += sizeof(struct dirhash_entry);
	dirhashsize += sizeof(struct dirhash_entry);
	LIST_INSERT_HEAD(&dirh->entries[hashline], dirh_e, next);
}


void
dirhash_enter_freed(struct dirhash *dirh, uint64_t offset,
	uint32_t entry_size)
{
	struct dirhash_entry *dirh_e;

	/* make sure we have a dirhash to work on */
	KASSERT(dirh);
	KASSERT(dirh->refcnt > 0);

#ifdef DEBUG
	/* check for double entry of free space */
	LIST_FOREACH(dirh_e, &dirh->free_entries, next)
		KASSERT(dirh_e->offset != offset);
#endif

	DPRINTF(("dirhash enter FREED %"PRIu64", %d\n",
		offset, entry_size));
	dirh_e = pool_get(&dirhash_entry_pool, PR_WAITOK);
	memset(dirh_e, 0, sizeof(struct dirhash_entry));

	dirh_e->hashvalue = 0;		/* not relevant */
	dirh_e->offset    = offset;
	dirh_e->d_namlen  = 0;		/* not relevant */
	dirh_e->entry_size  = entry_size;

	/* XXX it might be preferable to append them at the tail */
	LIST_INSERT_HEAD(&dirh->free_entries, dirh_e, next);
	dirh->size  += sizeof(struct dirhash_entry);
	dirhashsize += sizeof(struct dirhash_entry);
}


void
dirhash_remove(struct dirhash *dirh, struct dirent *dirent,
	uint64_t offset, uint32_t entry_size)
{
	struct dirhash_entry *dirh_e;
	uint32_t hashvalue, hashline;

	DPRINTF(("dirhash remove %"PRIu64", %d for `%*.*s`\n",
		offset, entry_size, 
		dirent->d_namlen, dirent->d_namlen, dirent->d_name));

	/* make sure we have a dirhash to work on */
	KASSERT(dirh);
	KASSERT(dirh->refcnt > 0);

	/* calculate our hash */
	hashvalue = dirhash_hash(dirent->d_name, dirent->d_namlen);
	hashline  = hashvalue & DIRHASH_HASHMASK;

	/* lookup entry */
	LIST_FOREACH(dirh_e, &dirh->entries[hashline], next) {
		/* check for hash collision */
		if (dirh_e->hashvalue != hashvalue)
			continue;
		if (dirh_e->offset != offset)
			continue;

		/* got it! */
		KASSERT(dirh_e->d_namlen == dirent->d_namlen);
		KASSERT(dirh_e->entry_size == entry_size);
		LIST_REMOVE(dirh_e, next);
		dirh->size      -= sizeof(struct dirhash_entry);
		dirhashsize -= sizeof(struct dirhash_entry);

		dirhash_enter_freed(dirh, offset, entry_size);
		return;
	}

	/* not found! */
	panic("dirhash_remove couldn't find entry in hash table\n");
}


/* BUGALERT: don't use result longer than needed, never past the node lock */
/* call with NULL *result initially and it will return nonzero if again */
int
dirhash_lookup(struct dirhash *dirh, const char *d_name, int d_namlen,
	struct dirhash_entry **result)
{
	struct dirhash_entry *dirh_e;
	uint32_t hashvalue, hashline;

	/* vnode should be locked */
	//KASSERT(VOP_ISLOCKED(dirh->vnode));

	/* make sure we have a dirhash to work on */
	KASSERT(dirh);
	KASSERT(dirh->refcnt > 0);

	/* start where we were */
	if (*result) {
		dirh_e = *result;

		/* retrieve information to avoid recalculation and advance */
		hashvalue = dirh_e->hashvalue;
		dirh_e = LIST_NEXT(*result, next);
	} else {
		/* calculate our hash and lookup all entries in hashline */
		hashvalue = dirhash_hash(d_name, d_namlen);
		hashline  = hashvalue & DIRHASH_HASHMASK;
		dirh_e = LIST_FIRST(&dirh->entries[hashline]);
	}

	for (; dirh_e; dirh_e = LIST_NEXT(dirh_e, next)) {
		/* check for hash collision */
		if (dirh_e->hashvalue != hashvalue)
			continue;
		if (dirh_e->d_namlen != d_namlen)
			continue;
		/* might have an entry in the cache */
		*result = dirh_e;
		return 1;
	}

	*result = NULL;
	return 0;
}


/* BUGALERT: don't use result longer than needed, never past the node lock */
/* call with NULL *result initially and it will return nonzero if again */
int
dirhash_lookup_freed(struct dirhash *dirh, uint32_t min_entrysize,
	struct dirhash_entry **result)
{
	struct dirhash_entry *dirh_e;

	//KASSERT(VOP_ISLOCKED(dirh->vnode));

	/* make sure we have a dirhash to work on */
	KASSERT(dirh);
	KASSERT(dirh->refcnt > 0);

	/* start where we were */
	if (*result) {
		dirh_e = LIST_NEXT(*result, next);
	} else {
		/* lookup all entries that match */
		dirh_e = LIST_FIRST(&dirh->free_entries);
	}

	for (; dirh_e; dirh_e = LIST_NEXT(dirh_e, next)) {
		/* check for minimum size */
		if (dirh_e->entry_size < min_entrysize)
			continue;
		/* might be a candidate */
		*result = dirh_e;
		return 1;
	}

	*result = NULL;
	return 0;
}


