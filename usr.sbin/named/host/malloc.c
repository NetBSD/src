/*	$NetBSD: malloc.c,v 1.2 1997/04/13 10:50:30 mrg Exp $	*/

/*
 * malloc/free/realloc memory management routines.
 *
 * This is a very simple, but fast storage allocator. It allocates blocks
 * of a small number of different sizes, and keeps free lists of each size.
 * Blocks that don't exactly fit are passed up to the next larger size.
 * In this implementation, the available sizes are (2^(i+4))-8 bytes long.
 * This is designed for use in a single-threaded virtual memory environment.
 *
 * This version is derived from: malloc.c (Caltech) 2/21/82, Chris Kingsley,
 * kingsley@cit-20, and a similar version of Larry Wall, used by perl.
 *
 * Rewritten by Eric Wassenaar, Nikhef-H, <e07@nikhef.nl>
 *
 * Not only varies the vendor-supplied implementation of this package greatly
 * for different platforms, there are also many subtle semantic differences,
 * especially for anomalous conditions. This makes it risky to use a separate
 * package, in case other library routines depend on those special features.
 * Nevertheless, we assume that external routines under normal circumstances
 * just need the basic malloc/free/realloc functionality.
 *
 * To avoid possible conflicts, included in this package are some auxiliary
 * functions: memalign, calloc/cfree, valloc/vfree, because they are closely
 * related to and built upon the basic functions.
 *
 * Not supported are special functions: mallopt/mallinfo/mstats, and those
 * which are found on specific platforms only: mallocblksize/recalloc (sgi),
 * malloc_debug/malloc_verify (sun), malloc_size/malloc_error (next).
 *
 * The simplicity of this package imposes the following limitations:
 * - Memory once allocated is never returned to the system, but is put on
 *   free lists for subsequent use. The process size grows monotonously.
 * - Allocating zero-size data blocks is not an error.
 * - In case realloc fails, the old block may no longer be allocated.
 * - The strictest alignment for memalign is the page size.
 */

#ifndef lint
static char Version[] = "@(#)malloc.c	e07@nikhef.nl (Eric Wassenaar) 961019";
#endif

#if defined(apollo) && defined(lint)
#define __attribute(x)
#endif

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>

#ifdef lint
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN int errno;

/*
 * Portability definitions. These are probably too primitive, but the
 * semantics on various platforms are too chaotic to do it correctly.
 */

#if defined(SYSV) || defined(SVR4)
#define SYSV_MALLOC
#define SYSV_MEMSET
#endif

#ifdef SYSV_MALLOC
typedef void	ptr_t;
typedef u_int	siz_t;
typedef void	free_t;
#define free_return(x)	return
#else
typedef char	ptr_t;
typedef u_int	siz_t;
typedef int	free_t;
#define free_return(x)	return(x)
#endif

#ifdef SYSV_MEMSET
#define bzero(a,n)	(void) memset(a,'\0',n)
#define bcopy(a,b,n)	(void) memcpy(b,a,n)
#endif

/*
 * The page size used to request blocks of system memory.
 */

#ifndef PAGESIZE
#ifdef NBPG

#ifdef CLSIZE
#define PAGESIZE (NBPG*CLSIZE)
#else
#define PAGESIZE NBPG
#endif

#else /*not NBPG*/

#ifdef NBPC
#define PAGESIZE NBPC
#else
#define PAGESIZE 4096		/* some reasonable default */
#endif

#endif /*NBPG*/
#endif

static int pagesize = 0;	/* page size after initialization */

/*
 * The overhead on a block is 8 bytes on traditional 32-bit platforms.
 * When free, this space contains a pointer to the next free block,
 * and the lower 3 bits of this pointer must be zero.
 * When in use, the first field is set to OVMAGIC with the lower 3 bits
 * nonzero, and the second field is the hash bucket index.
 * The overhead information precedes the data area returned to the user.
 * When special alignment is required, a fragment is created within the
 * data block, preceded by a special fragment header containing a magic
 * number FRMAGIC and the align offset from the overlapping block.
 */

typedef union overhead {

	struct {
		union overhead *ovfree_next;	/* next chunk on free list */
	} ov_free;

	struct {
		int ovused_magic;		/* magic number */
		int ovused_index;		/* bucket index */
	} ov_used;

	struct {
		int ovfrag_fmagic;		/* fragment magic number */
		int ovfrag_offset;		/* fragment align offset */
	} ov_frag;

	/* align on double word boundary */
	double	ov_align;

} OVHDR;

#define	ov_next		ov_free.ovfree_next	/* next on free list */
#define	ov_magic	ov_used.ovused_magic	/* magic number */
#define	ov_index	ov_used.ovused_index	/* bucket index */
#define	ov_fmagic	ov_frag.ovfrag_fmagic	/* fragment magic number */
#define	ov_offset	ov_frag.ovfrag_offset	/* fragment align offset */

#define	OVMAGIC		0xef			/* overhead magic number */
#define	FRMAGIC		0x5555			/* fragment magic number */
#define OVHDRSZ		sizeof(OVHDR)		/* size of overhead data */

/*
 * freelist[i] is the pointer to the next free chunk of size 2^(i+4).
 * The smallest allocatable chunk is 16 bytes, containing a user data
 * block of 8 bytes (assuming 8 bytes overhead on 32-bit platforms).
 */

#define MINCHUNK	4	/* minimum chunk size 2^(0+4)  */
#define MAXCHUNK	30	/* maximum chunk size 2^(26+4) */
#define	NBUCKETS	27	/* MAXCHUNK - MINCHUNK + 1 */

static OVHDR *freelist[NBUCKETS];	/* hash list of free chunks */
static int malloced[NBUCKETS];		/* number of allocated chunks */

#define chunk_size(i)	(1 << ((i) + MINCHUNK))
#define data_size(i)	(chunk_size(i) - OVHDRSZ)
#define MINDATA		((1 << MINCHUNK) - OVHDRSZ)
#define MAXDATA		((1 << MAXCHUNK) - OVHDRSZ)

#define get_offset(a,b)	((unsigned long)(a) & ((b) - 1))
#define word_offset(a)	get_offset(a, OVHDRSZ)
#define page_offset(a)	get_offset(a, PAGESIZE)
#define aligned(a,b)	(get_offset(a,b) == 0)
#define word_aligned(a)	aligned(a, OVHDRSZ)
#define page_aligned(a)	aligned(a, PAGESIZE)
#define auto_aligned(a)	aligned(a, a)

#define valid_index(i)	((i) >= 0 && (i) < NBUCKETS)
#define valid_offset(i)	((i) > OVHDRSZ && auto_aligned(i) && (i) <= PAGESIZE)

/*
 * Definition of modules.
 */

#define PROTO(TYPES)	()

ptr_t *malloc		PROTO((siz_t));
ptr_t *memalign		PROTO((siz_t, siz_t));
free_t free		PROTO((ptr_t *));
ptr_t *realloc		PROTO((ptr_t *, siz_t));
ptr_t *calloc		PROTO((siz_t, siz_t));
free_t cfree		PROTO((ptr_t *));
ptr_t *valloc		PROTO((siz_t));
free_t vfree		PROTO((ptr_t *));

extern ptr_t *sbrk	PROTO((int));

/*
** MALLOC -- Allocate more memory
** ------------------------------
*/

ptr_t *
malloc(size)
siz_t size;				/* amount of memory to allocate */
{
	register OVHDR *op;		/* chunk pointer */
	register int bucket;		/* hash bucket index */
	register int bucketsize;	/* size of hash bucket chunk */
	register int memsize;		/* amount of memory to expand */

/*
 * First time malloc is called, do some sanity checks, setup page size,
 * and align the break pointer so all chunk data will be page aligned.
 * Note. Cannot issue debugging print statements during initialization.
 */
	if (pagesize == 0)
	{
		if (page_offset(PAGESIZE) || word_offset(OVHDRSZ))
		{
			errno = EINVAL;
			return(NULL);
		}

		op = (OVHDR *)sbrk(0);
		if (op == NULL || (char *)op == (char *)-1)
		{
			errno = ENOMEM;
			return(NULL);
		}

		memsize = page_offset(op);
		if (memsize > 0)
		{
			memsize = PAGESIZE - memsize;
			op = (OVHDR *)sbrk(memsize);
			if (op == NULL || (char *)op == (char *)-1)
			{
				errno = ENOMEM;
				return(NULL);
			}
		}

		/* initialization complete */
		pagesize = PAGESIZE;
	}

/*
 * Convert amount of memory requested into closest chunk size
 * stored in hash buckets which satisfies request.
 */
	bucket = 0; bucketsize = chunk_size(bucket);
	while (bucketsize < OVHDRSZ || size > (bucketsize - OVHDRSZ))
	{
		bucket++; bucketsize <<= 1;
		if (bucket >= NBUCKETS)
		{
			errno = EINVAL;
			return(NULL);
		}
	}

/*
 * If nothing in hash bucket right now, request more memory from the system.
 * Add new memory allocated to that on free list for this hash bucket.
 * System memory is expanded by increments of whole pages. For small chunk
 * sizes, the page is subdivided into a list of free chunks.
 */
	if (freelist[bucket] == NULL)
	{
		memsize = (bucketsize < PAGESIZE) ? PAGESIZE : bucketsize;
		op = (OVHDR *)sbrk(memsize);
		if (op == NULL || (char *)op == (char *)-1)
		{
			errno = ENOMEM;
			return(NULL);
		}

		freelist[bucket] = op;
		while (memsize > bucketsize)
		{
			memsize -= bucketsize;
			op->ov_next = (OVHDR *)((char *)op + bucketsize);
			op = op->ov_next;
		}
		op->ov_next = NULL;
	}

/*
 * Memory is available.
 */
	/* remove from linked list */
	op = freelist[bucket];
	freelist[bucket] = op->ov_next;
	malloced[bucket]++;

	/* mark this chunk in use */
	op->ov_magic = OVMAGIC;
	op->ov_index = bucket;

	/* return pointer to user data block */
	return((ptr_t *)((char *)op + OVHDRSZ));
}

/*
** MEMALIGN -- Allocate memory with alignment constraints
** ------------------------------------------------------
*/

ptr_t *
memalign(align, size)
siz_t align;				/* required memory alignment */
siz_t size;				/* amount of memory to allocate */
{   
	register OVHDR *op;		/* chunk pointer */
	register ptr_t *newbuf;		/* new block of user data */
	register int offset = 0;	/* fragment offset for alignment */

/*
 * The alignment must be a power of two, and no bigger than the page size.
 */
	if ((align == 0) || !auto_aligned(align) || (align > PAGESIZE))
	{
		errno = EINVAL;
		return(NULL);
	}

/*
 * For ordinary small alignment sizes, we can use the plain malloc.
 */
	if (align > OVHDRSZ)
		offset = align - OVHDRSZ;

	newbuf = malloc(size + offset);
	if (newbuf == NULL)
		return(NULL);

/*
 * Otherwise we have to create a more strictly aligned fragment.
 */
	if (offset > 0)
	{
		/* locate the proper alignment boundary within the block */
		newbuf = (ptr_t *)((char *)newbuf + offset);

		/* mark this block as a special fragment */
		op = (OVHDR *)((char *)newbuf - OVHDRSZ);
		op->ov_fmagic = FRMAGIC;
		op->ov_offset = OVHDRSZ + offset;
	}

	return(newbuf);
}

/*
** DEALLOC -- Put unneeded memory on the free list
** -----------------------------------------------
*/

static int
dealloc(oldbuf)
ptr_t *oldbuf;				/* old block of user data */
{   
	register OVHDR *op;		/* chunk pointer */
	register int bucket;		/* hash bucket index */
	register int offset;		/* fragment offset for alignment */

	/* if no old block, or if not yet initialized */
	if (oldbuf == NULL || pagesize == 0)
	{
		errno = EINVAL;
		return(0);
	}

	/* avoid bogus block addresses */
	if (!word_aligned(oldbuf))
	{
		errno = EINVAL;
		return(0);
	}

	/* move to the header for this chunk */
	op = (OVHDR *)((char *)oldbuf - OVHDRSZ);

	/* adjust in case this is an aligned fragment */
	if ((op->ov_fmagic == FRMAGIC) && valid_offset(op->ov_offset))
	{
		offset = op->ov_offset - OVHDRSZ;
		op = (OVHDR *)((char *)op - offset);
	}

	/* check whether this chunk was really allocated */
	if ((op->ov_magic != OVMAGIC) || !valid_index(op->ov_index))
	{
		errno = EINVAL;
		return(0);
	}

	/* put back on the free list for this bucket */
	bucket = op->ov_index;
	op->ov_next = freelist[bucket];
	freelist[bucket] = op;
	malloced[bucket]--;

	return(1);
}

/*
** FREE -- Put unneeded memory on the free list
** --------------------------------------------
**
**	In this implementation, unneeded memory is never returned to
**	the system, but is put on a free memory hash list instead.
**	Subsequent malloc requests will first examine the free lists
**	to see whether a request can be satisfied.
**	As a consequence of this strategy, the process size will grow
**	monotonously, up to the largest amount needed at any moment.
**
**	On some platforms, this routine does not return a status code.
**	The status will be stored in a global variable ``free_status''
**	which can be examined if desired.
*/

int free_status = 0;			/* return code of last free */

free_t
free(oldbuf)
ptr_t *oldbuf;				/* old block of user data */
{   
	free_status = dealloc(oldbuf);
	free_return(free_status);
}

/*
** FINDBUCKET -- Locate a chunk of memory on the free list
** -------------------------------------------------------
**
**	When a program attempts "storage compaction" as mentioned in the
**	old malloc man page, it realloc's an already freed block. Usually
**	this is the last block it freed; occasionally it might be farther
**	back. We have to search all the free lists for the block in order
**	to determine its bucket: first we make one pass thru the lists
**	checking only the first block in each; if that fails we search
**	the entire lists for a match. If still not found it is an error.
*/

static int
findbucket(oldop)
OVHDR *oldop;				/* old chunk to search for */
{
	register OVHDR *op;		/* chunk pointer */
	register int bucket;		/* hash bucket index */

	for (bucket = 0; bucket < NBUCKETS; bucket++)
	{
		if (freelist[bucket] == oldop)
			return(bucket);
	}

	for (bucket = 0; bucket < NBUCKETS; bucket++)
	{
		for (op = freelist[bucket]; op != NULL; op = op->ov_next)
		{
			if (op == oldop)
				return(bucket);
		}
	}

	/* not found */
	return(-1);
}

/*
** REALLOC -- Rellocate already allocated memory
** ---------------------------------------------
*/

ptr_t *
realloc(oldbuf, size)
ptr_t *oldbuf;				/* old block of user data */
siz_t size;				/* amount of memory to allocate */
{   
	register OVHDR *op;		/* chunk pointer */
	register ptr_t *newbuf;		/* new block of user data */
	register int bucket;		/* hash bucket index */
	register int offset = 0;	/* fragment offset for alignment */
	siz_t minsize, maxsize;		/* size limits for this bucket */
	int allocated = 0;		/* set if old chunk was allocated */

/*
 * Do plain malloc if no old block, or if not yet initialized.
 * Otherwise, get the header, and check for special conditions.
 */
	if (oldbuf == NULL || pagesize == 0)
	{
		newbuf = malloc(size);
		return(newbuf);
	}

	/* avoid bogus block addresses */
	if (!word_aligned(oldbuf))
	{
		errno = EINVAL;
		return(NULL);
	}

	/* move to the header for this chunk */
	op = (OVHDR *)((char *)oldbuf - OVHDRSZ);

	/* adjust in case this is an aligned fragment */
	if ((op->ov_fmagic == FRMAGIC) && valid_offset(op->ov_offset))
	{
		offset = op->ov_offset - OVHDRSZ;
		op = (OVHDR *)((char *)op - offset);
	}

/*
 * Check whether this chunk is allocated at this moment.
 * If not, try to locate it on the free list hash buckets.
 */
	if ((op->ov_magic == OVMAGIC) && valid_index(op->ov_index))
	{
		allocated = 1;
		bucket = op->ov_index;
	}
	else
	{
		bucket = findbucket(op);
		if (bucket < 0)
		{
			errno = EINVAL;
			return(NULL);
		}
	}

/*
 * If the new size block fits into the same already allocated chunk,
 * we can just use it again, avoiding a malloc and a bcopy. Otherwise,
 * we can safely put it on the free list (the contents are preserved).
 */
	/* make sure the alignment offset is consistent with the bucket */
	if ((offset > 0) && (bucket == 0 || offset > data_size(bucket-1)))
	{
		errno = EINVAL;
		return(NULL);
	}

	/* maximum data block size of this chunk */
	maxsize = data_size(bucket) - offset;

	if (allocated)
	{
		/* maximum data block size in the preceding hash bucket */
		minsize = (bucket > 0) ? data_size(bucket-1) - offset : 0;

		/* re-use same block if within bounds */
		if (size <= maxsize && size > minsize)
			return(oldbuf);

		/* no longer useable */
		(void) dealloc(oldbuf);
	}

/*
 * A new chunk must be allocated, possibly with alignment restrictions.
 */
	newbuf = malloc(size + offset);
	if (newbuf == NULL)
		return(NULL);

	if (offset > 0)
	{
		/* locate the proper alignment boundary within the block */
		newbuf = (ptr_t *)((char *)newbuf + offset);

		/* mark this block as a special fragment */
		op = (OVHDR *)((char *)newbuf - OVHDRSZ);
		op->ov_fmagic = FRMAGIC;
		op->ov_offset = OVHDRSZ + offset;
	}

/*
 * Copy the contents of the old user data block into the new block.
 * In case we shrink, only copy the requested amount of user data.
 * If we expand, copy the maximum possible amount from the old block.
 * Note that the exact amount of valid old user data is not known.
 */
	if (oldbuf != newbuf)
		bcopy(oldbuf, newbuf, (size < maxsize) ? size : maxsize);

	return(newbuf);
}

/*
** CALLOC -- Allocate memory for number of elements, and clear it
** --------------------------------------------------------------
**
**	This is a wrapper for malloc to request memory for a number
**	of consecutive elements of certain length.
**	As a side effect, the entire memory block obtained is cleared.
*/

ptr_t *
calloc(count, length)
siz_t count;				/* number of elements */
siz_t length;				/* size per element */
{
	register ptr_t *newbuf;		/* new block of user data */
	register siz_t size;		/* amount of memory to allocate */

	size = count * length;
	newbuf = malloc(size);
	if (newbuf == NULL)
		return(NULL);

	bzero(newbuf, size);
	return(newbuf);
}


/*
** CFREE -- Put unneeded memory on the free list
** ---------------------------------------------
*/

free_t
cfree(oldbuf)
ptr_t *oldbuf;				/* old block of user data */
{   
	free_status = dealloc(oldbuf);
	free_return(free_status);
}

/*
** VALLOC -- Allocate memory on a page boundary
** --------------------------------------------
**
**	This is a wrapper for memalign to request memory that is
**	aligned on a page boundary. In this implementation, this
**	is the strictest alignment possible.
*/

ptr_t *
valloc(size)
siz_t size;				/* amount of memory to allocate */
{   
	register ptr_t *newbuf;		/* new block of user data */
	siz_t align = PAGESIZE;		/* alignment on page boundary */

	newbuf = memalign(align, size);
	return(newbuf);
}


/*
** VFREE -- Put unneeded memory on the free list
** ---------------------------------------------
*/

free_t
vfree(oldbuf)
ptr_t *oldbuf;				/* old block of user data */
{   
	free_status = dealloc(oldbuf);
	free_return(free_status);
}
