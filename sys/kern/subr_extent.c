/*	$NetBSD: subr_extent.c,v 1.1 1996/06/23 00:10:19 thorpej Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Matthias Drochner.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * General purpose extent manager.
 */

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/proc.h>

static	int extent_insert_and_optimize __P((struct extent *, u_long, u_long,
	    int, struct extent_region *));
static	struct extent_region *extent_alloc_region_descriptor
	    __P((struct extent *, int));
static	void extent_free_region_descriptor __P((struct extent *,
	    struct extent_region *));

/*
 * Allocate and initialize an extent map.
 */
struct extent *
extent_create(name, start, end, mtype, storage, storagesize, flags)
	char *name;
	u_long start, end;
	int mtype;
	caddr_t storage;
	size_t storagesize;
	int flags;
{
	struct extent *ex;
	caddr_t cp = storage;
	size_t sz = storagesize;
	struct extent_region *rp;
	int fixed_extent = (storage != NULL);

	/* Check arguments. */
	if (name == NULL)
		panic("extent_create: name == NULL");
	if (end < start) {
		printf("extent_create: extent `%s', start 0x%lx, end 0x%lx\n",
		    name, start, end);
		panic("extent_create: end < start");
	}
	if (fixed_extent && (storagesize < sizeof(struct extent_fixed)))
		panic("extent_create: fixed extent, bad storagesize 0x%x",
		    storagesize);

	/* Allocate extent descriptor. */
	if (fixed_extent) {
		struct extent_fixed *fex;

		bzero(storage, storagesize);

		/*
		 * Align all descriptors on "long" boundaries.
		 */
		fex = (struct extent_fixed *)cp;
		ex = (struct extent *)fex;
		cp += EXTENT_ALIGN(sizeof(struct extent_fixed), sizeof(long));
		sz -= EXTENT_ALIGN(sizeof(struct extent_fixed), sizeof(long));
		fex->fex_storage = storage;
		fex->fex_storagesize = storagesize;

		/*
		 * In a fixed extent, we have to pre-allocate region
		 * descriptors and place them in the extent's freelist.
		 */
		LIST_INIT(&fex->fex_freelist);
		while (sz >= sizeof(struct extent_region)) {
			rp = (struct extent_region *)cp;
			cp += EXTENT_ALIGN(sizeof(struct extent_region),
			    sizeof(long));
			sz -= EXTENT_ALIGN(sizeof(struct extent_region),
			    sizeof(long));
			LIST_INSERT_HEAD(&fex->fex_freelist, rp, er_link);
		}
	} else {
		ex = (struct extent *)malloc(sizeof(struct extent),
		    mtype, (flags & EX_WAITOK) ? M_WAITOK : M_NOWAIT);
		if (ex == NULL)
			return (NULL);
	}

	/* Fill in the extent descriptor and return it to the caller. */
	LIST_INIT(&ex->ex_regions);
	ex->ex_name = name;
	ex->ex_start = start;
	ex->ex_end = end;
	ex->ex_mtype = mtype;
	ex->ex_flags = 0;
	if (fixed_extent)
		ex->ex_flags |= EXF_FIXED;
	if (flags & EX_NOBLOB)
		ex->ex_flags |= EXF_NOBLOB;
	return (ex);
}

/*
 * Destroy an extent map.
 */
void
extent_destroy(ex)
	struct extent *ex;
{
	struct extent_region *rp, *orp;
	int mtype;

	/* Check arguments. */
	if (ex == NULL)
		panic("extent_destroy: NULL extent");

	mtype = ex->ex_mtype;

	/* Free all region descriptors in extent. */
	for (rp = ex->ex_regions.lh_first; rp != NULL; ) {
		orp = rp;
		rp = rp->er_link.le_next;
		LIST_REMOVE(orp, er_link);
		extent_free_region_descriptor(ex, orp);
	}

	/* If we're not a fixed extent, free the extent descriptor itself. */
	if ((ex->ex_flags & EXF_FIXED) == 0)
		free(ex, mtype);
}

/*
 * Insert a region descriptor into the sorted region list after the
 * entry "after" or at the head of the list (if "after" is NULL).
 */
static int
extent_insert_and_optimize(ex, start, size, flags, after)
	struct extent *ex;
	u_long start, size;
	int flags;
	struct extent_region *after;
{
	struct extent_region *rp;
	int appended = 0;

	if (after == NULL) {
		/*
		 * We're the first in the region list.  If there's
		 * a region after us, attempt to coalesce to save
		 * descriptor overhead.
		 */
		if (((ex->ex_flags & EXF_NOBLOB) == 0) &&
		    (ex->ex_regions.lh_first != NULL) &&
		    ((start + size) == ex->ex_regions.lh_first->er_start)) {
			/*
			 * We can coalesce.  Prepend us to the first region.
			 */
			ex->ex_regions.lh_first->er_start = start;
			return (0);
		}

		/*
		 * Can't coalesce.  Allocate a region descriptor, fill it
		 * in, and insert us at the head of the region list.
		 */
		rp = extent_alloc_region_descriptor(ex, flags);
		if (rp == NULL) {
#if defined(DEBUG) || defined(DIAGNOSTIC)
			printf("extent `%s': can't alloc region descriptor.\n",
			    ex->ex_name);
#endif
			return (ENOMEM);
		}
		rp->er_start = start;
		rp->er_end = start + (size - 1);
		LIST_INSERT_HEAD(&ex->ex_regions, rp, er_link);
		return (0);
	}

	/*
	 * If EXF_NOBLOB is set, coalescing is disallowed.
	 */
	if (ex->ex_flags & EXF_NOBLOB)
		goto allocate_region_descriptor;

	/*
	 * Attempt to coalesce with the region before us.
	 */
	if ((after->er_end + 1) == start) {
		/*
		 * We can coalesce.  Append ourselves and make
		 * note of it.
		 */
		after->er_end = start + (size - 1);
		appended = 1;
	}

	/*
	 * Attempt to coalesce with the region after us.
	 */
	if ((after->er_link.le_next != NULL) &&
	    ((start + size) == after->er_link.le_next->er_start)) {
		/*
		 * We can coalesce.  Note that if we appended ourselves
		 * to the previous region, we exactly fit the gap, and
		 * can free the "next" region descriptor.
		 */
		if (appended) {
			/*
			 * Yup, we can free it up.
			 */
			after->er_end = after->er_link.le_next->er_end;
			LIST_REMOVE(after->er_link.le_next, er_link);
			extent_free_region_descriptor(ex,
			    after->er_link.le_next);
		} else {
			/*
			 * Nope, just prepend us to the next region.
			 */
			after->er_link.le_next->er_start = start;
		}
		return (0);
	}

	/*
	 * We weren't able to coalesce with the next region, but
	 * we don't need to allocate a region descriptor if we
	 * appended ourselves to the previous region.
	 */
	if (appended)
		return (0);

 allocate_region_descriptor:

	/*
	 * Allocate a region descriptor and insert ourselves
	 * into the region list.
	 */
	rp = extent_alloc_region_descriptor(ex, flags);
	if (rp == NULL) {
#if defined(DEBUG) || defined(DIAGNOSTIC)
		printf("extent `%s': can't allocate region descriptor.\n",
		    ex->ex_name);
#endif
		return (ENOMEM);
	}
	rp->er_start = start;
	rp->er_end = start + (size - 1);
	LIST_INSERT_AFTER(after, rp, er_link);
	return (0);
}

/*
 * Allocate a specific region in an extent map.
 */
int
extent_alloc_region(ex, start, size, flags)
	struct extent *ex;
	u_long start, size;
	int flags;
{
	struct extent_region *rp, *last;
	u_long end = start + (size - 1);
	int error;

	/* Check arguments. */
	if (ex == NULL)
		panic("extent_alloc_region: NULL extent");
	if (size < 1) {
		printf("extent_alloc_region: extent `%s', size 0x%lx\n",
		    ex->ex_name, size);
		panic("extent_alloc_region: bad size");
	}
	if (end < start) {
		printf(
		 "extent_alloc_region: extent `%s', start 0x%lx, size 0x%lx\n",
		 ex->ex_name, start, size);
		panic("extent_alloc_region: overflow");
	}
	/*
	 * Make sure the requested region lies within the
	 * extent.
	 */
	if ((start < ex->ex_start) || (end > ex->ex_end)) {
		printf("extent_alloc_region: extent `%s' (0x%lx - 0x%lx)\n",
		    ex->ex_name, ex->ex_start, ex->ex_end);
		printf("extent_alloc_region: start 0x%lx, end 0x%lx\n",
		    start, end);
		panic("extent_alloc_region: region lies outside extent");
	}

 alloc_start:
	/*
	 * Attempt to place ourselves in the desired area of the
	 * extent.  We save ourselves some work by keeping the list sorted.
	 * In other words, if the start of the current region is greater
	 * than the end of our region, we don't have to search any further.
	 */

	/*
	 * Keep a pointer to the last region we looked at so
	 * that we don't have to traverse the list again when
	 * we insert ourselves.  If "last" is NULL when we
	 * finally insert ourselves, we go at the head of the
	 * list.  See extent_insert_and_optimize() for details.
	 */
	last = NULL;

	for (rp = ex->ex_regions.lh_first; rp != NULL;
	    rp = rp->er_link.le_next) {
		if (rp->er_start > end) {
			/*
			 * We lie before this region and don't
			 * conflict.
			 */
			 break;
		}

		/*
		 * The current region begins before we end.
		 * Check for a conflict.
		 */
		if (rp->er_end >= start) {
			/*
			 * We conflict.  If we can wait, do so.
			 */
			if (flags & EX_WAITOK) {
				ex->ex_flags |= EXF_WANTED;
				error = tsleep(ex,
				    PRIBIO | ((flags & EX_CATCH) ? PCATCH : 0),
				    "extnt", 0);
				if (error)
					return (error);
				goto alloc_start;
			}
			return (EAGAIN);
		}

		/*
		 * We don't conflict, but this region lies before
		 * us.  Keep a pointer to this region, and keep
		 * trying.
		 */
		last = rp;
	}

	/*
	 * We don't conflict with any regions.  "last" points
	 * to the region we fall after, or is NULL if we belong
	 * at the beginning of the region list.  Insert ourselves.
	 */
	return (extent_insert_and_optimize(ex, start, size, flags, last));
}

/*
 * Macro to check (x + y) <= z.  This check is designed to fail
 * if an overflow occurs.
 */
#define LE_OV(x, y, z)	((((x) + (y)) >= (x)) && (((x) + (y)) <= (z)))

/*
 * Allocate a region in an extent map subregion.
 *
 * If EX_FAST is specified, we return the first fit in the map.
 * Otherwise, we try to minimize fragmentation by finding the
 * smallest gap that will hold the request.
 *
 * The allocated region is aligned to "alignment", which must be
 * a power of 2.
 */
int
extent_alloc_subregion(ex, substart, subend, size, alignment, boundary,
    flags, result)
	struct extent *ex;
	u_long substart, subend, size, alignment, boundary;
	int flags;
	u_long *result;
{
	struct extent_region *rp, *last, *bestlast;
	u_long newstart, newend, beststart, bestovh, ovh;
	u_long dontcross, odontcross;
	int error;

	/* Check arguments. */
	if (ex == NULL)
		panic("extent_alloc_subregion: NULL extent");
	if (result == NULL)
		panic("extent_alloc_subregion: NULL result pointer");
	if ((substart < ex->ex_start) || (substart >= ex->ex_end) ||
	    (subend > ex->ex_end) || (subend <= ex->ex_start)) {
  printf("extent_alloc_subregion: extent `%s', ex_start 0x%lx, ex_end 0x%lx\n",
		    ex->ex_name, ex->ex_start, ex->ex_end);
		printf("extent_alloc_subregion: substart 0x%lx, subend 0x%lx\n",
		    substart, subend);
		panic("extent_alloc_subregion: bad subregion");
	}
	if ((size < 1) || (size > ((subend - substart) + 1))) {
		printf("extent_alloc_subregion: extent `%s', size 0x%lx\n",
		    ex->ex_name, size);
		panic("extent_alloc_subregion: bad size");
	}
	if (alignment == 0)
		panic("extent_alloc_subregion: bad alignment");
	if (boundary && (boundary < size)) {
		printf(
		    "extent_alloc_subregion: extent `%s', size 0x%lx,
		    boundary 0x%lx\n", ex->ex_name, size, boundary);
		panic("extent_alloc_subregion: bad boundary");
	}

 alloc_start:
	/*
	 * Keep a pointer to the last region we looked at so
	 * that we don't have to traverse the list again when
	 * we insert ourselves.  If "last" is NULL when we
	 * finally insert ourselves, we go at the head of the
	 * list.  See extent_insert_and_optimize() for deatails.
	 */
	last = NULL;

	/*
	 * Initialize the "don't cross" boundary, a.k.a a line
	 * that a region should not cross.  If the boundary lies
	 * before the region starts, we add the "boundary" argument
	 * until we get a meaningful comparison.
	 */
	dontcross = ex->ex_start + boundary;
	while (dontcross < substart)
		dontcross += boundary;

	/*
	 * Keep track of size and location of the smallest
	 * chunk we fit in.
	 *
	 * Since the extent can be as large as the numeric range
	 * of the CPU (0 - 0xffffffff for 32-bit systems), the
	 * best overhead value can be the maximum unsigned integer.
	 * Thus, we initialize "bestovh" to 0, since we insert ourselves
	 * into the region list immediately on an exact match (which
	 * is the only case where "bestovh" would be set to 0).
	 */
	bestovh = 0;
	beststart = 0;
	bestlast = NULL;

	/*
	 * For N allocated regions, we must make (N + 1)
	 * checks for unallocated space.  The first chunk we
	 * check is the area from the beginning of the subregion
	 * to the first allocated region.
	 */
	newstart = EXTENT_ALIGN(substart, alignment);
	if (newstart < ex->ex_start) {
		printf(
      "extent_alloc_subregion: extent `%s' (0x%lx - 0x%lx), alignment 0x%lx\n",
		 ex->ex_name, ex->ex_start, ex->ex_end, alignment);
		panic("extent_alloc_subregion: overflow after alignment");
	}

	for (rp = ex->ex_regions.lh_first; rp != NULL;
	    rp = rp->er_link.le_next) {
		/*
		 * Check the chunk before "rp".  Note that our
		 * comparison is safe from overflow conditions.
		 */
		if (LE_OV(newstart, size, rp->er_start)) {
			/*
			 * Do a boundary check, if necessary.  Note
			 * that a region may *begin* on the boundary,
			 * but it must end before the boundary.
			 */
			if (boundary) {
				newend = newstart + (size - 1);

				/*
				 * Adjust boundary for a meaningful
				 * comparison.
				 */
				while (dontcross <= newstart) {
					odontcross = dontcross;
					dontcross += boundary;

					/*
					 * If we run past the end of
					 * the extent or the boundary
					 * overflows, then the request
					 * can't fit.
					 */
					if ((dontcross > ex->ex_end) ||
					    (dontcross < odontcross))
						goto fail;
				}

				/* Do the boundary check. */
				if (newend >= dontcross) {
					/*
					 * Candidate region crosses
					 * boundary.  Try again.
					 */
					continue;
				}
			}

			/*
			 * We would fit into this space.  Calculate
			 * the overhead (wasted space).  If we exactly
			 * fit, or we're taking the first fit, insert
			 * ourselves into the region list.
			 */
			ovh = rp->er_start - newstart - size;
			if ((flags & EX_FAST) || (ovh == 0))
				goto found;

			/*
			 * Don't exactly fit, but check to see
			 * if we're better than any current choice.
			 */
			if ((bestovh == 0) || (ovh < bestovh)) {
				bestovh = ovh;
				beststart = newstart;
				bestlast = last;
			}
		}

		/*
		 * Skip past the current region and check again.
		 */
		newstart = EXTENT_ALIGN((rp->er_end + 1), alignment);
		if (newstart < rp->er_end) {
			/*
			 * Overflow condition.  Don't error out, since
			 * we might have a chunk of space that we can
			 * use.
			 */
			goto fail;
		}
		
		last = rp;
	}

	/*
	 * The final check is from the current starting point to the
	 * end of the subregion.  If there were no allocated regions,
	 * "newstart" is set to the beginning of the subregion, or
	 * just past the end of the last allocated region, adjusted
	 * for alignment in either case.
	 */
	if (LE_OV(newstart, (size - 1), subend)) {
		/*
		 * We would fit into this space.  Calculate
		 * the overhead (wasted space).  If we exactly
		 * fit, or we're taking the first fit, insert
		 * ourselves into the region list.
		 */
		ovh = ex->ex_end - newstart - (size - 1);
		if ((flags & EX_FAST) || (ovh == 0))
			goto found;

		/*
		 * Don't exactly fit, but check to see
		 * if we're better than any current choice.
		 */
		if ((bestovh == 0) || (ovh < bestovh)) {
			bestovh = ovh;
			beststart = newstart;
			bestlast = last;
		}
	}

 fail:
	/*
	 * One of the following two conditions have
	 * occurred:
	 *
	 *	There is no chunk large enough to hold the request.
	 *
	 *	If EX_FAST was not specified, there is not an
	 *	exact match for the request.
	 *
	 * Note that if we reach this point and EX_FAST is
	 * set, then we know there is no space in the extent for
	 * the request.
	 */
	if (((flags & EX_FAST) == 0) && (bestovh != 0)) {
		/*
		 * We have a match that's "good enough".
		 */
		newstart = beststart;
		last = bestlast;
		goto found;
	}

	/*
	 * No space currently available.  Wait for it to free up,
	 * if possible.
	 */
	if (flags & EX_WAITOK) {
		ex->ex_flags |= EXF_WANTED;
		error = tsleep(ex,
		    PRIBIO | ((flags & EX_CATCH) ? PCATCH : 0), "extnt", 0);
		if (error)
			return (error);
		goto alloc_start;
	}

	return (EAGAIN);

 found:
	/*
	 * Insert ourselves into the region list.
	 */
	error = extent_insert_and_optimize(ex, newstart, size, flags, last);
	if (error == 0)
		*result = newstart;
	return (error);
}

int
extent_free(ex, start, size, flags)
	struct extent *ex;
	u_long start, size;
	int flags;
{
	struct extent_region *rp;
	u_long end = start + (size - 1);

	/* Check arguments. */
	if (ex == NULL)
		panic("extent_free: NULL extent");
	if ((start < ex->ex_start) || (start > ex->ex_end)) {
		extent_print(ex);
		printf("extent_free: extent `%s', start 0x%lx, size 0x%lx\n",
		    ex->ex_name, start, size);
		panic("extent_free: extent `%s', region not within extent",
		    ex->ex_name);
	}
	/* Check for an overflow. */
	if (end < start) {
		extent_print(ex);
		printf("extent_free: extent `%s', start 0x%lx, size 0x%lx\n",
		    ex->ex_name, start, size);
		panic("extent_free: overflow");
	}

	/*
	 * Find region and deallocate.  Several possibilities:
	 *
	 *	1. (start == er_start) && (end == er_end):
	 *	   Free descriptor.
	 *
	 *	2. (start == er_start) && (end < er_end):
	 *	   Adjust er_start.
	 *
	 *	3. (start > er_start) && (end == er_end):
	 *	   Adjust er_end.
	 *
	 *	4. (start > er_start) && (end < er_end):
	 *	   Fragment region.  Requires descriptor alloc.
	 *
	 * Cases 2, 3, and 4 require that the EXF_NOBLOB flag
	 * is not set.
	 */
	for (rp = ex->ex_regions.lh_first; rp != NULL;
	    rp = rp->er_link.le_next) {
		/*
		 * Save ourselves some comparisons; does the current
		 * region end before chunk to be freed begins?  If so,
		 * then we haven't found the appropriate region descriptor.
		 */
		if (rp->er_end < start)
			continue;

		/*
		 * Save ourselves some traversal; does the current
		 * region begin after the chunk to be freed ends?  If so,
		 * then we've already passed any possible region descriptors
		 * that might have contained the chunk to be freed.
		 */
		if (rp->er_start > end)
			break;

		/* Case 1. */
		if ((start == rp->er_start) && (end == rp->er_end)) {
			LIST_REMOVE(rp, er_link);
			extent_free_region_descriptor(ex, rp);
			goto done;
		}

		/*
		 * The following cases all require that EXF_NOBLOB
		 * is not set.
		 */
		if (ex->ex_flags & EXF_NOBLOB)
			continue;

		/* Case 2. */
		if ((start == rp->er_start) && (end < rp->er_end)) {
			rp->er_start = (end + 1);
			goto done;
		}

		/* Case 3. */
		if ((start > rp->er_start) && (end == rp->er_end)) {
			rp->er_end = (start - 1);
			goto done;
		}

		/* Case 4. */
		if ((start > rp->er_start) && (end < rp->er_end)) {
			struct extent_region *nrp;

			/* Allocate a region descriptor. */
			nrp = extent_alloc_region_descriptor(ex, flags);
			if (nrp == NULL)
				return (ENOMEM);

			/* Fill in new descriptor. */
			nrp->er_start = end + 1;
			nrp->er_end = rp->er_end;

			/* Adjust current descriptor. */
			rp->er_end = start - 1;

			/* Instert new descriptor after current. */
			LIST_INSERT_AFTER(rp, nrp, er_link);
			goto done;
		}
	}

	/* Region not found, or request otherwise invalid. */
	extent_print(ex);
	printf("extent_free: start 0x%lx, end 0x%lx\n", start, end);
	panic("extent_free: region not found");

 done:
	if (ex->ex_flags & EXF_WANTED) {
		ex->ex_flags &= ~EXF_WANTED;
		wakeup(ex);
	}
	return (0);
}

static struct extent_region *
extent_alloc_region_descriptor(ex, flags)
	struct extent *ex;
	int flags;
{
	struct extent_region *rp;
	int s;

	if (ex->ex_flags & EXF_FIXED) {
		struct extent_fixed *fex = (struct extent_fixed *)ex;

		while (fex->fex_freelist.lh_first == NULL) {
			if (flags & EX_MALLOCOK)
				goto alloc;

			if ((flags & EX_WAITOK) == 0)
				return (NULL);
			ex->ex_flags |= EXF_FLWANTED;
			if (tsleep(&fex->fex_freelist,
			    PRIBIO | ((flags & EX_CATCH) ? PCATCH : 0),
			    "extnt", 0))
				return (NULL);
		}
		/* Atomic. */
		s = splhigh();
		rp = fex->fex_freelist.lh_first;
		LIST_REMOVE(rp, er_link);
		splx(s);

		rp->er_flags = 0;

		return (rp);
	}

 alloc:
	rp = (struct extent_region *)
	    malloc(sizeof(struct extent_region), ex->ex_mtype,
	    (flags & EX_WAITOK) ? M_WAITOK : M_NOWAIT);

	rp->er_flags = ER_ALLOC;

	return (rp);
}

static void
extent_free_region_descriptor(ex, rp)
	struct extent *ex;
	struct extent_region *rp;
{

	if ((rp->er_flags & ER_ALLOC) == 0) {
		struct extent_fixed *fex = (struct extent_fixed *)ex;

		LIST_INSERT_HEAD(&fex->fex_freelist, rp, er_link);
		if (ex->ex_flags & EXF_FLWANTED) {
			ex->ex_flags &= ~EXF_FLWANTED;
			wakeup(&fex->fex_freelist);
		}
	} else
		free(rp, ex->ex_mtype);
}

void
extent_print(ex)
	struct extent *ex;
{
	struct extent_region *rp;

	if (ex == NULL)
		panic("extent_print: NULL extent");

	printf("extent `%s' (0x%lx - 0x%lx), flags = 0x%x\n", ex->ex_name,
	    ex->ex_start, ex->ex_end, ex->ex_flags);

	for (rp = ex->ex_regions.lh_first; rp != NULL;
	    rp = rp->er_link.le_next)
		printf("     0x%lx - 0x%lx\n", rp->er_start, rp->er_end);
}
