/*	$NetBSD: vm_swap.c,v 1.37.2.13 1997/05/11 07:58:09 mrg Exp $	*/

/*
 * Copyright (c) 1995, 1996, 1997 Matthew R. Green
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
 *      This product includes software developed by Matthew R. Green for
 *      The NetBSD Foundation.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/dmap.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/map.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/extent.h>
#include <vm/vm_swap.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <machine/vmparam.h>

#include <vm/vm_conf.h>

#include <miscfs/specfs/specdev.h>

/*
 * The idea here is to provide a single interface for
 * multiple swap devices, of any kind and priority
 * in a simple and fast way.
 *
 * Each swap device has several properties:
 *	* rate of use.
 *	* swap in use.
 *	* swap enabled.
 *
 * And the rate in use is determined by XXX.
 *
 * The arguments to swap(2) are:
 *	int cmd;
 *	void *arg;
 *	int misc;
 * The cmd can be one of:
 *	SWAP_NSWAP - swap(2) returns the number of swap devices
 *		currently in use. (done)
 *	SWAP_STATS - swap(2) takes a struct ent * in (void *arg)
 *		and writes misc or fewer (to zero) entries of
 *		configured swap devices, and returns the number of
 *		entries written.  -1 on error (EFAULT, EPERM, ..)
 *		(done).
 *	SWAP_ON - swap(2) takes a (char *) in arg to be the pathname
 *		of a device or file to begin swapping on, with it's
 *		priority in misc, returning 0 on success and -1 on
 *		failure, setting errno to indicate which (done).
 *	SWAP_OFF - swap(2) takes a (char *) n arg to be the pathname
 *		of a device or file to stop swapping on.  returning 0
 *		or -1 (done).  XXX unwritten
 *	SWAP_CTL - swap(2) changes the properties of a swap device,
 *		using the misc value (done).
 */

/*
 * XXX
 *
 * Does all the manipulation of the swap_priority list, etc, need to
 * be locked ?
 */

#define SWAPDEBUG

#ifdef SWAPDEBUG
#define	VMSDB_SWON	0x0001
#define VMSDB_SWOFF	0x0002
#define VMSDB_SWINIT	0x0004
#define VMSDB_SWALLOC	0x0008
#define VMSDB_SWFLOW	0x0010
#define VMSDB_INFO	0x0020
int vmswapdebug = VMSDB_SWON | VMSDB_SWOFF | VMSDB_SWINIT | VMSDB_SWALLOC |
    VMSDB_SWFLOW | VMSDB_INFO;
#endif

struct swapdev {
	struct swapent		swd_se;
#define swd_dev			swd_se.se_dev
#define swd_flags		swd_se.se_flags
#define swd_nblks		swd_se.se_nblks
#define swd_inuse		swd_se.se_inuse
#define swd_priority		swd_se.se_priority
	daddr_t			swd_mapoffset;
	int			swd_mapsize;
	struct extent		*swd_ex;
	struct vnode		*swd_vp;
	CIRCLEQ_ENTRY(swapdev)	swd_next;
};

struct swappri {
	int			spi_priority;
	CIRCLEQ_HEAD(spi_swapdev, swapdev)	spi_swapdev;
	LIST_ENTRY(swappri)	spi_swappri;
};

int nswapdev, nswap;
int swflags;
struct extent *swapmap;
LIST_HEAD(swap_priority, swappri) swap_priority;

static int swap_on __P((struct proc *, struct swapdev *));
#ifdef SWAP_OFF_WORKS
static int swap_off __P((struct proc *, struct swapdev *));
#endif
static struct swapdev *swap_getdevfromaddr __P((daddr_t));
static void swap_addmap __P((struct swapdev *, int));

int
sys_swapon(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_swapon_args /* {
		syscallarg(int) cmd;
		syscallarg(void *) arg;
		syscallarg(int) misc;
	} */ *uap = (struct sys_swapon_args *)v;
	struct vnode *vp;
	struct nameidata nd;
	struct swappri *spp;
	struct swapdev *sdp = NULL;
	struct swapent *sep;
	int	count, error, misc;

	misc = SCARG(uap, misc);

#ifdef SWAPDEBUG
	if (vmswapdebug & VMSDB_SWFLOW)
		printf("entering sys_swapon\n");
#endif /* SWAPDEBUG */
	
	/* how many swap devices */
	if (SCARG(uap, cmd) == SWAP_NSWAP) {
#ifdef SWAPDEBUG
		if (vmswapdebug & VMSDB_SWFLOW)
			printf("did SWAP_NSWAP:  leaving sys_swapon\n");
#endif /* SWAPDEBUG */
		*retval = nswapdev;
		return (0);
	}

	/* stats on the swap devices. */
	if (SCARG(uap, cmd) == SWAP_STATS) {
		sep = (struct swapent *)SCARG(uap, arg);
		count = 0;

		for (spp = swap_priority.lh_first; spp != NULL;
		    spp = spp->spi_swappri.le_next) {
			for (sdp = spp->spi_swapdev.cqh_first;
			     sdp != (void *)&spp->spi_swapdev && misc-- > 0;
			     sdp = sdp->swd_next.cqe_next) {
				error = copyout((caddr_t)&sdp->swd_se,
				    (caddr_t)sep, sizeof(struct swapent));
				if (error)
					return (error);
				count++;
				sep++;
			}
		}
#ifdef SWAPDEBUG
		if (vmswapdebug & VMSDB_SWFLOW)
			printf("sw: did SWAP_STATS:  leaving sys_swapon\n");
#endif /* SWAPDEBUG */
		*retval = count;
		return (0);
	} 
	if ((error = suser(p->p_ucred, &p->p_acflag)))
		return (error);

	NDINIT(&nd, LOOKUP, FOLLOW|LOCKLEAF, UIO_USERSPACE, SCARG(uap, arg), p);
	if ((error = namei(&nd)))
		return (error);

	vp = nd.ni_vp;

	switch(SCARG(uap, cmd)) {
	case SWAP_ON:
	{
		int	priority = SCARG(uap, misc);
		struct	swappri *nspp = NULL;
		struct	swapdev *nsdp = NULL;
		struct	swappri *pspp;

#ifdef SWAPDEBUG
		if (vmswapdebug & VMSDB_SWFLOW)
			printf("sw: doing SWAP_ON...\n");
#endif /* SWAPDEBUG */

		pspp = swap_priority.lh_first;

		/* Check for duplicates */
		for (spp = pspp; spp != NULL; spp = spp->spi_swappri.le_next) {
			for (sdp = spp->spi_swapdev.cqh_first;
			     sdp != (void *)&spp->spi_swapdev;
			     sdp = sdp->swd_next.cqe_next)
				if (sdp->swd_vp == vp) {
					error = EBUSY;
					goto bad;
				}
		}

		for (spp = pspp; spp != NULL; spp = spp->spi_swappri.le_next) {
			if (spp->spi_priority <= priority)
				break;
			pspp = spp;
		}

		if (spp == NULL || spp->spi_priority != priority) {
			nspp = (struct swappri *)
				malloc(sizeof *nspp, M_VMSWAP, M_WAITOK);

#ifdef SWAPDEBUG
			if (vmswapdebug & VMSDB_SWFLOW)
				printf("sw: had to create a new swappri = %d\n",
				   priority);
#endif /* SWAPDEBUG */

			nspp->spi_priority = priority;
			CIRCLEQ_INIT(&nspp->spi_swapdev);

			if (pspp)
				LIST_INSERT_AFTER(pspp, nspp, spi_swappri);
			else
				LIST_INSERT_HEAD(&swap_priority, nspp,
						 spi_swappri);

			spp = nspp;
		}

		nsdp = (struct swapdev *)malloc(sizeof *nsdp, M_VMSWAP,
					        M_WAITOK);
		nsdp->swd_inuse = nsdp->swd_flags = 0;
		nsdp->swd_priority = priority;
		nsdp->swd_vp = vp;
		nsdp->swd_dev = (vp->v_type == VBLK) ? vp->v_rdev : NODEV;
		if ((error = swap_on(p, nsdp)) != 0) {
			free((caddr_t)nsdp, M_VMSWAP);
			if (nspp) {
				LIST_REMOVE(nspp, spi_swappri);
				free((caddr_t)nspp, M_VMSWAP);
			}
			break;
		}

		/* Onto priority list */
		CIRCLEQ_INSERT_TAIL(&spp->spi_swapdev, nsdp, swd_next);

		/* Keep reference to vnode */
		vref(vp);
		break;
	}

	case SWAP_OFF:
#ifdef SWAPDEBUG
		if (vmswapdebug & VMSDB_SWFLOW)
			printf("doing SWAP_OFF...\n");
#endif /* SWAPDEBUG */
#ifdef SWAP_OFF_WORKS
		for (spp = swap_priority.lh_first; spp != NULL;
		     spp = spp->spi_swappri.le_next) {
			for (sdp = spp->spi_swapdev.cqh_first;
			     sdp != (void *)&spp->spi_swapdev;
			     sdp = sdp->swd_next.cqe_next) {
				if (sdp->swd_vp != vp)
					continue;
				/*
				 * if a device isn't in use or enabled, we
				 * can't stop swapping from it (again).
				 */
				if ((sdp->swd_flags & (SWF_INUSE|SWF_ENABLE)) == 0) {
					error = EBUSY;
					goto bad;
				}
				if ((error = swap_off(p, sdp)) != 0)
					goto bad;
				CIRCLEQ_REMOVE(&spp->spi_swapdev, sdp, swd_next);
				free((caddr_t)sdp, M_VMSWAP);
			}
		}
		if (sdp == NULL)
			error = ENXIO;
#else
#ifdef DIAGNOSTIC
		printf("swap SWAP_OFF attempted\n");
#endif
#endif
		break;

	case SWAP_CTL:
#ifdef SWAPDEBUG
		if (vmswapdebug & VMSDB_SWFLOW)
			printf("doing SWAP_CTL...\n");
#endif /* SWAPDEBUG */

	default:
#ifdef SWAPDEBUG
		if (vmswapdebug & VMSDB_SWFLOW)
			printf("doing default...\n");
#endif /* SWAPDEBUG */
		error = EINVAL;
	}

bad:
	vput(vp);

#ifdef SWAPDEBUG
	if (vmswapdebug & VMSDB_SWFLOW)
		printf("leaving sys_swapon:  error %d\n", error);
#endif /* SWAPDEBUG */
	return (error);
}

/*
 * swap_on() attempts to begin swapping on a swapdev.  we check that this
 * device is OK to swap from, miss the start of any disk (to avoid any
 * disk labels that may exist).
 */
static int
swap_on(p, sdp)
	struct proc *p;
	struct swapdev *sdp;
{
	static int count = 0;
	struct vnode *vp = sdp->swd_vp;
	int error, nblks, size;
	long addr;
	char *storage;
	int storagesize;
#define SWAP_TO_FILES
#ifdef SWAP_TO_FILES
	struct vattr va;
#endif
	dev_t dev = sdp->swd_dev;
	char *name;

#if 0
	/*
	 * XXX
	 *
	 * Need to handle where root is on swap.
	 */
	if (vp != rootvp) {
		if ((error = VOP_OPEN(vp, FREAD|FWRITE, p->p_ucred, p)))
			return (error);
	}
#endif

#ifdef SWAPDEBUG	/* this wants only for block devices */
	if (vmswapdebug & VMSDB_INFO)
		printf("swap_on: dev = %d, major(dev) = %d\n", dev, major(dev));
#endif /* SWAPDEBUG */

	if (vp->v_type == VBLK) {
		if (bdevsw[major(dev)].d_psize == 0 ||
		    (nblks = (*bdevsw[major(dev)].d_psize)(dev)) == -1) {
			error = ENXIO;
			goto bad;
		}
	} else {
#ifdef SWAP_TO_FILES
		if ((error = VOP_GETATTR(vp, &va, p->p_ucred, p)))
			goto bad;
		nblks = (int)btodb(va.va_size);
#else
		error = ENXIO;
		goto bad;
#endif
	}
	if (nblks == 0) {
#ifdef SWAPDEBUG
		if (vmswapdebug & VMSDB_SWFLOW)
			printf("swap_on: nblks == 0\n");
#endif /* SWAPDEBUG */
		error = EINVAL;
		goto bad;
	}

	sdp->swd_flags |= SWF_INUSE;
	sdp->swd_nblks = nblks;

	/*
	 * skip over first cluster of a device in case of labels or
	 * boot blocks.
	 */
	if (vp->v_type == VBLK) {
		size = (int)(nblks - ctod(CLSIZE));
		addr = (long)ctod(CLSIZE);
	} else {
		size = (int)nblks;
		addr = (long)0;
	}

#ifdef SWAPDEBUG
	if (vmswapdebug & VMSDB_SWON)
		printf("swap_on: dev %x: size %d, addr %ld\n", dev, size, addr);
#endif /* SWAPDEBUG */

	name = malloc(12, M_VMSWAP, M_WAITOK);
	sprintf(name, "swap0x%04x", count++);
	/* XXX make this based on ram as well. */
	storagesize = EXTENT_FIXED_STORAGE_SIZE(maxproc * 2);
	storage = malloc(storagesize, M_VMSWAP, M_WAITOK);
	sdp->swd_ex = extent_create(name, addr, addr + size, M_VMSWAP,
				    storage, storagesize,
				    EX_NOCOALESCE|EX_WAITOK);
	swap_addmap(sdp, size);
	nswapdev++;
	nswap += nblks;
	sdp->swd_flags |= SWF_ENABLE;
	if (dumpdev == NULL && vp->v_type == VBLK)
		dumpdev = dev;

	/* XXX handle miniroot == (rootvp == vp) */
	return (0);

bad:
	/* XXX see above */
#if 0
	if (vp != rootvp)
		(void)VOP_CLOSE(vp, FREAD|FWRITE, p->p_ucred, p);
#endif
	return (error);
}

#ifdef SWAP_OFF_WORKS
static int
swap_off(p, sdp)
	struct proc *p;
	struct swapdev *sdp;
{
	/* turn off the enable flag */
	sdp->swd_flags &= ~SWF_ENABLE;

#ifdef SWAPDEBUG
	if (vmswapdebug & VMSDB_SWOFF)
		printf("swap_off: %x\n", sdp->swd_dev);
#endif /* SWAPDEBUG */

	/*
	 * XXX write me
	 *
	 * the idea is to find out which processes are using this swap
	 * device, and page them all in.
	 *
	 * eventually, we should try to move them out to other swap areas
	 * if available.
	 */

	/* until the above code is written, we must ENODEV */
	return ENODEV;

	free(sdp->swd_ex->ex_name, M_VMSWAP);
	extent_free(swapmap, sdp->swd_mapoffset, sdp->swd_mapsize, EX_WAITOK);
	nswap -= sdp->swd_nblks;
	nswapdev--;
	extent_destroy(sdp->swd_ex);
	free((caddr_t)sdp->swd_ex, M_VMSWAP);
	if (sdp->swp_vp != rootvp)
		(void) VOP_CLOSE(sdp->swd_vp, FREAD|FWRITE, p->p_ucred, p);
	if (sdp->swd_vp)
		vrele(sdp->swd_vp);
	free((caddr_t)sdp, M_VMSWAP);
	return (0);
}
#endif

/*
 * to decide where to allocate what part of swap, we must "round robin"
 * the swap devicies in swap_priority of the same priority until they are
 * full.  we do this with a list of swap priorities that have circle
 * queues of swapdevs.
 *
 * the following functions control allocation and freeing of part of the
 * swap area.  you call swap_alloc() with a size and it returns an address.
 * later you call swap_free() and it frees the use of that swap area.
 *
 *	daddr_t swap_alloc(int size);
 *	void swap_free(int size, daddr_t addr);
 */

daddr_t
swap_alloc(size)
	int size;
{
	struct swapdev *sdp;
	struct swappri *spp;
	u_long	result;

	if (nswapdev < 1)
		return 0;
	
	/*
	 * XXX
	 * should we lock the swap_priority list and each swap device
	 * in turn while doing this search ?
	 */

	for (spp = swap_priority.lh_first; spp != NULL;
	     spp = spp->spi_swappri.le_next) {
		for (sdp = spp->spi_swapdev.cqh_first;
		     sdp != (void *)&spp->spi_swapdev;
		     sdp = sdp->swd_next.cqe_next) {
			/* if it's not enabled, then we can't swap from it */
			if ((sdp->swd_flags & SWF_ENABLE) == 0 ||
			    extent_alloc(sdp->swd_ex, size, EX_NOALIGN,
					 EX_NOBOUNDARY, EX_MALLOCOK|EX_NOWAIT,
					 &result) != 0) {
				/*
				 * XXX
				 * do something smart to note this partition
				 * as being full, yadda yadda yadda...
				 *
				 * perhaps check swd_nblks == swd_inuse ?
				 */
				continue;
			}
			CIRCLEQ_REMOVE(&spp->spi_swapdev, sdp, swd_next);
			CIRCLEQ_INSERT_TAIL(&spp->spi_swapdev, sdp, swd_next);
			sdp->swd_inuse += size;
			return (daddr_t)(result + sdp->swd_mapoffset);
		}
	}
	return 0;
}

void
swap_free(size, addr)
	int size;
	daddr_t addr;
{
	struct swapdev *sdp = swap_getdevfromaddr(addr);

#ifdef DIAGNOSTIC
	if (sdp == NULL)
		panic("swap_free: unmapped address\n");
	if (nswapdev < 1)
		panic("swap_free: nswapdev < 1\n");
#endif
	extent_free(sdp->swd_ex, addr - sdp->swd_mapoffset, size,
		    EX_MALLOCOK|EX_NOWAIT);
	sdp->swd_inuse -= size;
#ifdef DIAGNOSTIC
	if (sdp->swd_inuse < 0)
		panic("swap_free: inuse < 0");
#endif
}

/*
 * We have a physical -> virtual mapping to address here.  There
 * are several different physical address spaces (one for each
 * swap partition) that are to be mapped onto a single virtual
 * address space.
 */
#define ADDR_IN_MAP(addr, sdp) \
	(((addr) >= (sdp)->swd_mapoffset) && \
 	 ((addr) < ((sdp)->swd_mapoffset + (sdp)->swd_mapsize)))

struct swapdev *
swap_getdevfromaddr(addr)
	daddr_t addr;
{
	struct swapdev *sdp;
	struct swappri *spp;
	
	for (spp = swap_priority.lh_first; spp != NULL;
	     spp = spp->spi_swappri.le_next)
		for (sdp = spp->spi_swapdev.cqh_first;
		     sdp != (void *)&spp->spi_swapdev;
		     sdp = sdp->swd_next.cqe_next)
			if (ADDR_IN_MAP(addr, sdp))
				return sdp;
	return NULL;
}

void
swap_addmap(sdp, size)
	struct swapdev *sdp;
	int	size;
{
	u_long result;

	if (extent_alloc(swapmap, size, EX_NOALIGN, EX_NOBOUNDARY,
			 EX_WAITOK, &result))
		panic("swap_addmap");

	sdp->swd_mapoffset = result;
	sdp->swd_mapsize = size;
}

/*ARGSUSED*/
int
swread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(swstrategy, NULL, dev, B_READ, minphys, uio));
}

/*ARGSUSED*/
int
swwrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(swstrategy, NULL, dev, B_WRITE, minphys, uio));
}

void
swstrategy(bp)
	struct buf *bp;
{
	struct swapdev *sdp;
	struct vnode *vp;
	daddr_t addr;

	addr = bp->b_blkno;
	sdp = swap_getdevfromaddr(addr);
	if (sdp == NULL) {
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}
	bp->b_lblkno = bp->b_blkno = addr - sdp->swd_mapoffset;

#ifdef SWAPDEBUG
	if (vmswapdebug & VMSDB_SWFLOW)
		printf("swstrategy(%s): mapoff %x, addr %x, blkno %d\n",
			((bp->b_flags & B_READ) == 0) ? "write" : "read",
			sdp->swd_mapoffset, addr, bp->b_lblkno);
#endif

	VHOLD(sdp->swd_vp);
	if ((bp->b_flags & B_READ) == 0) {
		if ((vp = bp->b_vp) != NULL) {
			vp->v_numoutput--;
			if ((vp->v_flag & VBWAIT) && vp->v_numoutput <= 0) {
				vp->v_flag &= ~VBWAIT;
				wakeup((caddr_t)&vp->v_numoutput);
			}
		}
		sdp->swd_vp->v_numoutput++;
	}

	if (bp->b_vp != NULL)
		brelvp(bp);
	bp->b_vp = sdp->swd_vp;
	if (bp->b_vp->v_type == VBLK)
		bp->b_dev = sdp->swd_dev;
	VOP_STRATEGY(bp);
}

void
swapinit()
{
	struct buf *sp = swbuf;
	struct proc *p = &proc0;       /* XXX */
	u_long size, addr;
	int i;

#ifdef SWAPDEBUG
	if (vmswapdebug & VMSDB_SWINIT)
		printf("swapinit\n");
#endif
	nswap = 0;
	nswapdev = 0;

	LIST_INIT(&swap_priority);
	/*
	 * XXX
	 * this is really just wanting to create a Large Address Space
	 * that we can map each swap devive into.  if VM_{MIN,MAX}_ADDRESS
	 * are bad things to use here, change it!
	 */
	size = VM_MAX_ADDRESS - VM_MIN_ADDRESS;
	addr = VM_MIN_ADDRESS;
	if (addr == 0)
		addr = 0x1000;	/* So we can't have 0 as a valid address */
#ifdef SWAPDEBUG
	if (vmswapdebug & VMSDB_SWALLOC)
		printf("swapinit: swapmap size=%08lx addr=%08lx\n", size, addr);
#endif
	swapmap = extent_create("swapmap", addr, addr + size,
				M_VMSWAP, 0, 0, EX_WAITOK);
	if (swapmap == 0)
		panic("swapinit: extent_create failed");

	/*
	 * Now set up swap buffer headers.
	 */
	bswlist.b_actf = sp;
	for (i = 0; i < nswbuf - 1; i++, sp++) {
		sp->b_actf = sp + 1;
		sp->b_rcred = sp->b_wcred = p->p_ucred;
		sp->b_vnbufs.le_next = NOLIST;
	}
	sp->b_rcred = sp->b_wcred = p->p_ucred;
	sp->b_vnbufs.le_next = NOLIST;
	sp->b_actf = NULL;
#ifdef SWAPDEBUG
	if (vmswapdebug & VMSDB_SWINIT)
		printf("leaving swapinit\n");
#endif
}
