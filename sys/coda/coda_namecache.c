/*	$NetBSD: coda_namecache.c,v 1.26 2014/10/18 08:33:27 snj Exp $	*/

/*
 *
 *             Coda: an Experimental Distributed File System
 *                              Release 3.1
 *
 *           Copyright (c) 1987-1998 Carnegie Mellon University
 *                          All Rights Reserved
 *
 * Permission  to  use, copy, modify and distribute this software and its
 * documentation is hereby granted,  provided  that  both  the  copyright
 * notice  and  this  permission  notice  appear  in  all  copies  of the
 * software, derivative works or  modified  versions,  and  any  portions
 * thereof, and that both notices appear in supporting documentation, and
 * that credit is given to Carnegie Mellon University  in  all  documents
 * and publicity pertaining to direct or indirect use of this code or its
 * derivatives.
 *
 * CODA IS AN EXPERIMENTAL SOFTWARE SYSTEM AND IS  KNOWN  TO  HAVE  BUGS,
 * SOME  OF  WHICH MAY HAVE SERIOUS CONSEQUENCES.  CARNEGIE MELLON ALLOWS
 * FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION.   CARNEGIE  MELLON
 * DISCLAIMS  ANY  LIABILITY  OF  ANY  KIND  FOR  ANY  DAMAGES WHATSOEVER
 * RESULTING DIRECTLY OR INDIRECTLY FROM THE USE OF THIS SOFTWARE  OR  OF
 * ANY DERIVATIVE WORK.
 *
 * Carnegie  Mellon  encourages  users  of  this  software  to return any
 * improvements or extensions that  they  make,  and  to  grant  Carnegie
 * Mellon the rights to redistribute these changes without encumbrance.
 *
 * 	@(#) coda/coda_namecache.c,v 1.1.1.1 1998/08/29 21:26:45 rvb Exp $
 */

/*
 * Mach Operating System
 * Copyright (c) 1990 Carnegie-Mellon University
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

/*
 * This code was written for the Coda file system at Carnegie Mellon University.
 * Contributers include David Steere, James Kistler, and M. Satyanarayanan.
 */

/*
 * This module contains the routines to implement the CODA name cache. The
 * purpose of this cache is to reduce the cost of translating pathnames
 * into Vice FIDs. Each entry in the cache contains the name of the file,
 * the vnode (FID) of the parent directory, and the cred structure of the
 * user accessing the file.
 *
 * The first time a file is accessed, it is looked up by the local Venus
 * which first insures that the user has access to the file. In addition
 * we are guaranteed that Venus will invalidate any name cache entries in
 * case the user no longer should be able to access the file. For these
 * reasons we do not need to keep access list information as well as a
 * cred structure for each entry.
 *
 * The table can be accessed through the routines cnc_init(), cnc_enter(),
 * cnc_lookup(), cnc_rmfidcred(), cnc_rmfid(), cnc_rmcred(), and cnc_purge().
 * There are several other routines which aid in the implementation of the
 * hash table.
 */

/*
 * NOTES: rvb@cs
 * 1.	The name cache holds a reference to every vnode in it.  Hence files can not be
 *	 closed or made inactive until they are released.
 * 2.	coda_nc_name(cp) was added to get a name for a cnode pointer for debugging.
 * 3.	coda_nc_find() has debug code to detect when entries are stored with different
 *	 credentials.  We don't understand yet, if/how entries are NOT EQ but still
 *	 EQUAL
 * 4.	I wonder if this name cache could be replace by the vnode name cache.
 *	The latter has no zapping functions, so probably not.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: coda_namecache.c,v 1.26 2014/10/18 08:33:27 snj Exp $");

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/select.h>
#include <sys/kauth.h>

#include <coda/coda.h>
#include <coda/cnode.h>
#include <coda/coda_namecache.h>
#include <coda/coda_subr.h>

/*
 * Declaration of the name cache data structure.
 */

int 	coda_nc_use = 1;			 /* Indicate use of CODA Name Cache */

int	coda_nc_size = CODA_NC_CACHESIZE;	 /* size of the cache */
int	coda_nc_hashsize = CODA_NC_HASHSIZE; /* size of the primary hash */

struct 	coda_cache *coda_nc_heap;	/* pointer to the cache entries */
struct	coda_hash  *coda_nc_hash;	/* hash table of cfscache pointers */
struct	coda_lru   coda_nc_lru;		/* head of lru chain */

struct coda_nc_statistics coda_nc_stat;	/* Keep various stats */

/*
 * for testing purposes
 */
int coda_nc_debug = 0;

/*
 * Entry points for the CODA Name Cache
 */
static struct coda_cache *
coda_nc_find(struct cnode *dcp, const char *name, int namelen,
	kauth_cred_t cred, int hash);
static void
coda_nc_remove(struct coda_cache *cncp, enum dc_status dcstat);

/*
 * Initialize the cache, the LRU structure and the Hash structure(s)
 */

#define TOTAL_CACHE_SIZE 	(sizeof(struct coda_cache) * coda_nc_size)
#define TOTAL_HASH_SIZE 	(sizeof(struct coda_hash)  * coda_nc_hashsize)

int coda_nc_initialized = 0;      /* Initially the cache has not been initialized */

void
coda_nc_init(void)
{
    int i;

    /* zero the statistics structure */

    memset(&coda_nc_stat, 0, (sizeof(struct coda_nc_statistics)));

#ifdef	CODA_VERBOSE
    printf("CODA NAME CACHE: CACHE %d, HASH TBL %d\n", CODA_NC_CACHESIZE, CODA_NC_HASHSIZE);
#endif
    CODA_ALLOC(coda_nc_heap, struct coda_cache *, TOTAL_CACHE_SIZE);
    CODA_ALLOC(coda_nc_hash, struct coda_hash *, TOTAL_HASH_SIZE);

    memset(coda_nc_heap, 0, TOTAL_CACHE_SIZE);
    memset(coda_nc_hash, 0, TOTAL_HASH_SIZE);

    TAILQ_INIT(&coda_nc_lru.head);

    for (i=0; i < coda_nc_size; i++) {	/* initialize the heap */
	TAILQ_INSERT_HEAD(&coda_nc_lru.head, &coda_nc_heap[i], lru);
    }

    for (i=0; i < coda_nc_hashsize; i++) {	/* initialize the hashtable */
	LIST_INIT(&coda_nc_hash[i].head);
    }

    coda_nc_initialized++;
}

/*
 * Auxillary routines -- shouldn't be entry points
 */

static struct coda_cache *
coda_nc_find(struct cnode *dcp, const char *name, int namelen,
	kauth_cred_t cred, int hash)
{
	/*
	 * hash to find the appropriate bucket, look through the chain
	 * for the right entry (especially right cred, unless cred == 0)
	 */
	struct coda_cache *cncp;
	int count = 1;

	CODA_NC_DEBUG(CODA_NC_FIND,
		myprintf(("coda_nc_find(dcp %p, name %s, len %d, cred %p, hash %d\n",
			dcp, name, namelen, cred, hash));)

	LIST_FOREACH(cncp, &coda_nc_hash[hash].head, hash)
	{

	    if ((CODA_NAMEMATCH(cncp, name, namelen, dcp)) &&
		((cred == 0) || (cncp->cred == cred)))
	    {
		/* compare cr_uid instead */
		coda_nc_stat.Search_len += count;
		return(cncp);
	    }
#ifdef	DEBUG
	    else if (CODA_NAMEMATCH(cncp, name, namelen, dcp)) {
	    	printf("coda_nc_find: name %s, new cred = %p, cred = %p\n",
			name, cred, cncp->cred);
		printf("nref %d, nuid %d, ngid %d // oref %d, ocred %d, ogid %d\n",
			kauth_cred_getrefcnt(cred),
			kauth_cred_geteuid(cred),
			kauth_cred_getegid(cred),
			kauth_cred_getrefcnt(cncp->cred),
			kauth_cred_geteuid(cncp->cred),
			kauth_cred_getegid(cncp->cred));
		coda_print_cred(cred);
		coda_print_cred(cncp->cred);
	    }
#endif
	    count++;
	}

	return((struct coda_cache *)0);
}

/*
 * Enter a new (dir cnode, name) pair into the cache, updating the
 * LRU and Hash as needed.
 */
void
coda_nc_enter(struct cnode *dcp, const char *name, int namelen,
	kauth_cred_t cred, struct cnode *cp)
{
    struct coda_cache *cncp;
    int hash;

    if (coda_nc_use == 0)			/* Cache is off */
	return;

    CODA_NC_DEBUG(CODA_NC_ENTER,
		myprintf(("Enter: dcp %p cp %p name %s cred %p \n",
		       dcp, cp, name, cred)); )

    if (namelen > CODA_NC_NAMELEN) {
	CODA_NC_DEBUG(CODA_NC_ENTER,
		    myprintf(("long name enter %s\n",name));)
	    coda_nc_stat.long_name_enters++;	/* record stats */
	return;
    }

    hash = CODA_NC_HASH(name, namelen, dcp);
    cncp = coda_nc_find(dcp, name, namelen, cred, hash);
    if (cncp != (struct coda_cache *) 0) {
	coda_nc_stat.dbl_enters++;		/* duplicate entry */
	return;
    }

    coda_nc_stat.enters++;		/* record the enters statistic */

    /* Grab the next element in the lru chain */
    cncp = TAILQ_FIRST(&coda_nc_lru.head);
    TAILQ_REMOVE(&coda_nc_lru.head, cncp, lru);

    if (CODA_NC_VALID(cncp)) {
	/* Seems really ugly, but we have to decrement the appropriate
	   hash bucket length here, so we have to find the hash bucket
	   */
	coda_nc_hash[CODA_NC_HASH(cncp->name, cncp->namelen, cncp->dcp)].length--;

	coda_nc_stat.lru_rm++;	/* zapped a valid entry */
	LIST_REMOVE(cncp, hash);
	vrele(CTOV(cncp->dcp));
	vrele(CTOV(cncp->cp));
	kauth_cred_free(cncp->cred);
    }

    /*
     * Put a hold on the current vnodes and fill in the cache entry.
     */
    vref(CTOV(cp));
    vref(CTOV(dcp));
    kauth_cred_hold(cred);
    cncp->dcp = dcp;
    cncp->cp = cp;
    cncp->namelen = namelen;
    cncp->cred = cred;

    memcpy(cncp->name, name, (unsigned)namelen);

    /* Insert into the lru and hash chains. */
    TAILQ_INSERT_TAIL(&coda_nc_lru.head, cncp, lru);
    LIST_INSERT_HEAD(&coda_nc_hash[hash].head, cncp, hash);
    coda_nc_hash[hash].length++;                      /* Used for tuning */

    CODA_NC_DEBUG(CODA_NC_PRINTCODA_NC, print_coda_nc(); )
}

/*
 * Find the (dir cnode, name) pair in the cache, if its cred
 * matches the input, return it, otherwise return 0
 */
struct cnode *
coda_nc_lookup(struct cnode *dcp, const char *name, int namelen,
	kauth_cred_t cred)
{
	int hash;
	struct coda_cache *cncp;

	if (coda_nc_use == 0)			/* Cache is off */
		return((struct cnode *) 0);

	if (namelen > CODA_NC_NAMELEN) {
	        CODA_NC_DEBUG(CODA_NC_LOOKUP,
			    myprintf(("long name lookup %s\n",name));)
		coda_nc_stat.long_name_lookups++;		/* record stats */
		return((struct cnode *) 0);
	}

	/* Use the hash function to locate the starting point,
	   then the search routine to go down the list looking for
	   the correct cred.
 	 */

	hash = CODA_NC_HASH(name, namelen, dcp);
	cncp = coda_nc_find(dcp, name, namelen, cred, hash);
	if (cncp == (struct coda_cache *) 0) {
		coda_nc_stat.misses++;			/* record miss */
		return((struct cnode *) 0);
	}

	coda_nc_stat.hits++;

	/* put this entry at the end of the LRU */
	TAILQ_REMOVE(&coda_nc_lru.head, cncp, lru);
	TAILQ_INSERT_TAIL(&coda_nc_lru.head, cncp, lru);

	/* move it to the front of the hash chain */
	/* don't need to change the hash bucket length */
	LIST_REMOVE(cncp, hash);
	LIST_INSERT_HEAD(&coda_nc_hash[hash].head, cncp, hash);

	CODA_NC_DEBUG(CODA_NC_LOOKUP,
		printf("lookup: dcp %p, name %s, cred %p = cp %p\n",
			dcp, name, cred, cncp->cp); )

	return(cncp->cp);
}

static void
coda_nc_remove(struct coda_cache *cncp, enum dc_status dcstat)
{
	/*
	 * remove an entry -- vrele(cncp->dcp, cp), crfree(cred),
	 * remove it from its hash chain, and
	 * place it at the head of the lru list.
	 */
        CODA_NC_DEBUG(CODA_NC_REMOVE,
		    myprintf(("coda_nc_remove %s from parent %s\n",
			      cncp->name, coda_f2s(&cncp->dcp->c_fid))); )


	LIST_REMOVE(cncp, hash);
	memset(&cncp->hash, 0, sizeof(cncp->hash));

	if ((dcstat == IS_DOWNCALL) && (CTOV(cncp->dcp)->v_usecount == 1)) {
		cncp->dcp->c_flags |= C_PURGING;
	}
	vrele(CTOV(cncp->dcp));

	if ((dcstat == IS_DOWNCALL) && (CTOV(cncp->cp)->v_usecount == 1)) {
		cncp->cp->c_flags |= C_PURGING;
	}
	vrele(CTOV(cncp->cp));

	kauth_cred_free(cncp->cred);
	memset(DATA_PART(cncp), 0, DATA_SIZE);

	/* move the null entry to the front for reuse */
	TAILQ_REMOVE(&coda_nc_lru.head, cncp, lru);
	TAILQ_INSERT_HEAD(&coda_nc_lru.head, cncp, lru);
}

/*
 * Remove all entries with a parent which has the input fid.
 */
void
coda_nc_zapParentfid(CodaFid *fid, enum dc_status dcstat)
{
	/* To get to a specific fid, we might either have another hashing
	   function or do a sequential search through the cache for the
	   appropriate entries. The later may be acceptable since I don't
	   think callbacks or whatever Case 1 covers are frequent occurrences.
	 */
	struct coda_cache *cncp, *ncncp;
	int i;

	if (coda_nc_use == 0)			/* Cache is off */
		return;

	CODA_NC_DEBUG(CODA_NC_ZAPPFID,
		myprintf(("ZapParent: fid %s\n", coda_f2s(fid))); )

	coda_nc_stat.zapPfids++;

	for (i = 0; i < coda_nc_hashsize; i++) {

		/*
		 * Need to save the hash_next pointer in case we remove the
		 * entry. remove causes hash_next to point to itself.
		 */

		ncncp = LIST_FIRST(&coda_nc_hash[i].head);
		while ((cncp = ncncp) != NULL) {
			ncncp = LIST_NEXT(cncp, hash);

			if (coda_fid_eq(&(cncp->dcp->c_fid), fid)) {
			        coda_nc_hash[i].length--;      /* Used for tuning */
				coda_nc_remove(cncp, dcstat);
			}
		}
	}
}

/*
 * Remove all entries which have the same fid as the input
 */
void
coda_nc_zapfid(CodaFid *fid, enum dc_status dcstat)
{
	/* See comment for zapParentfid. This routine will be used
	   if attributes are being cached.
	 */
	struct coda_cache *cncp, *ncncp;
	int i;

	if (coda_nc_use == 0)			/* Cache is off */
		return;

	CODA_NC_DEBUG(CODA_NC_ZAPFID,
		myprintf(("Zapfid: fid %s\n", coda_f2s(fid))); )

	coda_nc_stat.zapFids++;

	for (i = 0; i < coda_nc_hashsize; i++) {

		ncncp = LIST_FIRST(&coda_nc_hash[i].head);
		while ((cncp = ncncp) != NULL) {
			ncncp = LIST_NEXT(cncp, hash);

			if (coda_fid_eq(&cncp->cp->c_fid, fid)) {
			        coda_nc_hash[i].length--;     /* Used for tuning */
				coda_nc_remove(cncp, dcstat);
			}
		}
	}
}

/*
 * Remove all entries which match the fid and the cred
 */
void
coda_nc_zapvnode(CodaFid *fid, kauth_cred_t cred,
    enum dc_status dcstat)
{
	/* See comment for zapfid. I don't think that one would ever
	   want to zap a file with a specific cred from the kernel.
	   We'll leave this one unimplemented.
	 */
	if (coda_nc_use == 0)			/* Cache is off */
		return;

	CODA_NC_DEBUG(CODA_NC_ZAPVNODE,
		myprintf(("Zapvnode: fid %s cred %p\n",
			  coda_f2s(fid), cred)); )
}

/*
 * Remove all entries which have the (dir vnode, name) pair
 */
void
coda_nc_zapfile(struct cnode *dcp, const char *name, int namelen)
{
	/* use the hash function to locate the file, then zap all
 	   entries of it regardless of the cred.
	 */
	struct coda_cache *cncp;
	int hash;

	if (coda_nc_use == 0)			/* Cache is off */
		return;

	CODA_NC_DEBUG(CODA_NC_ZAPFILE,
		myprintf(("Zapfile: dcp %p name %s \n",
			  dcp, name)); )

	if (namelen > CODA_NC_NAMELEN) {
		coda_nc_stat.long_remove++;		/* record stats */
		return;
	}

	coda_nc_stat.zapFile++;

	hash = CODA_NC_HASH(name, namelen, dcp);
	cncp = coda_nc_find(dcp, name, namelen, 0, hash);

	while (cncp) {
	  coda_nc_hash[hash].length--;                 /* Used for tuning */
/* 1.3 */
	  coda_nc_remove(cncp, NOT_DOWNCALL);
	  cncp = coda_nc_find(dcp, name, namelen, 0, hash);
	}
}

/*
 * Remove all the entries for a particular user. Used when tokens expire.
 * A user is determined by his/her effective user id (id_uid).
 */
void
coda_nc_purge_user(uid_t uid, enum dc_status dcstat)
{
	/*
	 * I think the best approach is to go through the entire cache
	 * via HASH or whatever and zap all entries which match the
	 * input cred. Or just flush the whole cache.  It might be
	 * best to go through on basis of LRU since cache will almost
	 * always be full and LRU is more straightforward.
	 */

	struct coda_cache *cncp, *ncncp;
	int hash;

	if (coda_nc_use == 0)			/* Cache is off */
		return;

	CODA_NC_DEBUG(CODA_NC_PURGEUSER,
		myprintf(("ZapDude: uid %x\n", uid)); )
	coda_nc_stat.zapUsers++;

	ncncp = TAILQ_FIRST(&coda_nc_lru.head);
	while ((cncp = ncncp) != NULL) {
		ncncp = TAILQ_NEXT(cncp, lru);

		if ((CODA_NC_VALID(cncp)) &&
		   (kauth_cred_geteuid(cncp->cred) == uid)) {
		        /* Seems really ugly, but we have to decrement the appropriate
			   hash bucket length here, so we have to find the hash bucket
			   */
		        hash = CODA_NC_HASH(cncp->name, cncp->namelen, cncp->dcp);
			coda_nc_hash[hash].length--;     /* For performance tuning */

			coda_nc_remove(cncp, dcstat);
		}
	}
}

/*
 * Flush the entire name cache. In response to a flush of the Venus cache.
 */
void
coda_nc_flush(enum dc_status dcstat)
{
	/* One option is to deallocate the current name cache and
	   call init to start again. Or just deallocate, then rebuild.
	   Or again, we could just go through the array and zero the
	   appropriate fields.
	 */

	/*
	 * Go through the whole lru chain and kill everything as we go.
	 * I don't use remove since that would rebuild the lru chain
	 * as it went and that seemed unneccesary.
	 */
	struct coda_cache *cncp;
	int i;

	if (coda_nc_use == 0)			/* Cache is off */
		return;

	coda_nc_stat.Flushes++;

	TAILQ_FOREACH(cncp, &coda_nc_lru.head, lru) {
		if (CODA_NC_VALID(cncp)) {	/* only zero valid nodes */
			LIST_REMOVE(cncp, hash);
			memset(&cncp->hash, 0, sizeof(cncp->hash));

			if ((dcstat == IS_DOWNCALL)
			    && (CTOV(cncp->dcp)->v_usecount == 1))
			{
				cncp->dcp->c_flags |= C_PURGING;
			}
			vrele(CTOV(cncp->dcp));

			if (CTOV(cncp->cp)->v_iflag & VI_TEXT) {
			    if (coda_vmflush(cncp->cp))
				CODADEBUG(CODA_FLUSH,
					myprintf(("coda_nc_flush: %s busy\n",
						coda_f2s(&cncp->cp->c_fid))); )
			}

			if ((dcstat == IS_DOWNCALL)
			    && (CTOV(cncp->cp)->v_usecount == 1))
			{
				cncp->cp->c_flags |= C_PURGING;
			}
			vrele(CTOV(cncp->cp));

			kauth_cred_free(cncp->cred);
			memset(DATA_PART(cncp), 0, DATA_SIZE);
		}
	}

	for (i = 0; i < coda_nc_hashsize; i++)
	  coda_nc_hash[i].length = 0;
}

/*
 * Debugging routines
 */

/*
 * This routine should print out all the hash chains to the console.
 */
void
print_coda_nc(void)
{
	int hash;
	struct coda_cache *cncp;

	for (hash = 0; hash < coda_nc_hashsize; hash++) {
		myprintf(("\nhash %d\n",hash));

		LIST_FOREACH(cncp, &coda_nc_hash[hash].head, hash) {
			myprintf(("cp %p dcp %p cred %p name %s\n",
				  cncp->cp, cncp->dcp,
				  cncp->cred, cncp->name));
		     }
	}
}

void
coda_nc_gather_stats(void)
{
    int i, xmax = 0, sum = 0, temp, zeros = 0, ave, n;

	for (i = 0; i < coda_nc_hashsize; i++) {
	  if (coda_nc_hash[i].length) {
	    sum += coda_nc_hash[i].length;
	  } else {
	    zeros++;
	  }

	  if (coda_nc_hash[i].length > xmax)
	    xmax = coda_nc_hash[i].length;
	}

	/*
	 * When computing the Arithmetic mean, only count slots which
	 * are not empty in the distribution.
	 */
        coda_nc_stat.Sum_bucket_len = sum;
        coda_nc_stat.Num_zero_len = zeros;
        coda_nc_stat.Max_bucket_len = xmax;

	if ((n = coda_nc_hashsize - zeros) > 0)
	  ave = sum / n;
	else
	  ave = 0;

	sum = 0;
	for (i = 0; i < coda_nc_hashsize; i++) {
	  if (coda_nc_hash[i].length) {
	    temp = coda_nc_hash[i].length - ave;
	    sum += temp * temp;
	  }
	}
        coda_nc_stat.Sum2_bucket_len = sum;
}

/*
 * The purpose of this routine is to allow the hash and cache sizes to be
 * changed dynamically. This should only be used in controlled environments,
 * it makes no effort to lock other users from accessing the cache while it
 * is in an improper state (except by turning the cache off).
 */
int
coda_nc_resize(int hashsize, int heapsize, enum dc_status dcstat)
{
    if ((hashsize % 2) || (heapsize % 2)) { /* Illegal hash or cache sizes */
	return(EINVAL);
    }

    coda_nc_use = 0;                       /* Turn the cache off */

    coda_nc_flush(dcstat);                 /* free any cnodes in the cache */

    /* WARNING: free must happen *before* size is reset */
    CODA_FREE(coda_nc_heap,TOTAL_CACHE_SIZE);
    CODA_FREE(coda_nc_hash,TOTAL_HASH_SIZE);

    coda_nc_hashsize = hashsize;
    coda_nc_size = heapsize;

    coda_nc_init();                        /* Set up a cache with the new size */

    coda_nc_use = 1;                       /* Turn the cache back on */
    return(0);
}

char coda_nc_name_buf[CODA_MAXNAMLEN+1];

void
coda_nc_name(struct cnode *cp)
{
	struct coda_cache *cncp;
	int i;

	if (coda_nc_use == 0)			/* Cache is off */
		return;

	for (i = 0; i < coda_nc_hashsize; i++) {

		LIST_FOREACH(cncp, &coda_nc_hash[i].head, hash) {
			if (cncp->cp == cp) {
				memcpy(coda_nc_name_buf, cncp->name, cncp->namelen);
				coda_nc_name_buf[cncp->namelen] = 0;
				printf(" is %s (%p,%p)@%p",
					coda_nc_name_buf, cncp->cp, cncp->dcp, cncp);
			}

		}
	}
}
