/*
 * Copyright (c) 1982, 1986, 1989 Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)isofs_inode.c
 *	$Id: isofs_node.c,v 1.6 1993/09/07 15:40:55 ws Exp $
 */

#include "param.h"
#include "systm.h"
#include "mount.h"
#include "proc.h"
#include "file.h"
#include "buf.h"
#include "vnode.h"
#include "kernel.h"
#include "malloc.h"
#include "stat.h"

#include "iso.h"
#include "isofs_node.h"
#include "iso_rrip.h"

#define	IFTOVT(mode)	(iftovt_tab[((mode) & 0170000) >> 12])
static enum vtype iftovt_tab[16] = {
    VNON, VFIFO, VCHR, VNON, VDIR, VNON, VBLK, VNON,
    VREG, VNON, VLNK, VNON, VSOCK, VNON, VNON, VBAD,
};

#define	INOHSZ	512
#if	((INOHSZ&(INOHSZ-1)) == 0)
#define	INOHASH(dev,ino)	(((dev)+(ino))&(INOHSZ-1))
#else
#define	INOHASH(dev,ino)	(((unsigned)((dev)+(ino)))%INOHSZ)
#endif

union iso_ihead {
	union  iso_ihead *ih_head[2];
	struct iso_node *ih_chain[2];
} iso_ihead[INOHSZ];

#ifdef	ISODEVMAP
#define	DNOHSZ	64
#if	((DNOHSZ&(DNOHSZ-1)) == 0)
#define	DNOHASH(dev,ino)	(((dev)+(ino))&(DNOHSZ-1))
#else
#define	DNOHASH(dev,ino)	(((unsigned)((dev)+(ino)))%DNOHSZ)
#endif

union iso_dhead {
	union  iso_dhead  *dh_head[2];
	struct iso_dnode *dh_chain[2];
} iso_dhead[DNOHSZ];
#endif

int prtactive;	/* 1 => print out reclaim of active vnodes */

/*
 * Initialize hash links for inodes and dnodes.
 */
isofs_init()
{
	register int i;
	register union iso_ihead *ih = iso_ihead;
#ifdef	ISODEVMAP
	register union iso_dhead *dh = iso_dhead;
#endif

#ifndef lint
	if (VN_MAXPRIVATE < sizeof(struct iso_node))
		panic("ihinit: too small");
#endif /* not lint */
	for (i = INOHSZ; --i >= 0; ih++) {
		ih->ih_head[0] = ih;
		ih->ih_head[1] = ih;
	}
#ifdef	ISODEVMAP
	for (i = DNOHSZ; --i >= 0; dh++) {
		dh->dh_head[0] = dh;
		dh->dh_head[1] = dh;
	}
#endif
}

#ifdef	ISODEVMAP
/*
 * Enter a new node into the device hash list
 */
struct iso_dnode *
iso_dmap(dev,ino,create)
	dev_t	dev;
	ino_t	ino;
	int	create;
{
	struct iso_dnode *dp;
	union iso_dhead *dh;
	
	dh = &iso_dhead[DNOHASH(dev, ino)];
	for (dp = dh->dh_chain[0];
	     dp != (struct iso_dnode *)dh;
	     dp = dp->d_forw)
		if (ino == dp->i_number && dev == dp->i_dev)
			return dp;

	if (!create)
		return (struct iso_dnode *)0;

	MALLOC(dp,struct iso_dnode *,sizeof(struct iso_dnode),M_CACHE,M_WAITOK);
	dp->i_dev = dev;
	dp->i_number = ino;
	insque(dp,dh);
	
	return dp;
}

void
iso_dunmap(dev)
	dev_t	dev;
{
	struct iso_dnode *dp, *dq;
	union iso_dhead *dh;
	
	for (dh = iso_dhead; dh < iso_dhead + DNOHSZ; dh++) {
		for (dp = dh->dh_chain[0];
		     dp != (struct iso_dnode *)dh;
		     dp = dq) {
			dq = dp->d_forw;
			if (dev == dp->i_dev) {
				remque(dp);
				FREE(dp,M_CACHE);
			}
		}
	}
}
#endif

/*
 * Look up a ISOFS dinode number to find its incore vnode.
 * If it is not in core, read it in from the specified device.
 * If it is in core, wait for the lock bit to clear, then
 * return the inode locked. Detection and handling of mount
 * points must be done by the calling routine.
 */
iso_iget(xp, ino, ipp, isodir)
	struct iso_node *xp;
	ino_t ino;
	struct iso_node **ipp;
	struct iso_directory_record *isodir;
{
	dev_t dev = xp->i_dev;
	struct mount *mntp = ITOV(xp)->v_mount;
	extern struct vnodeops isofs_vnodeops, isofs_spec_inodeops;
	register struct iso_node *ip, *iq;
	register struct vnode *vp;
	register struct iso_dnode *dp;
	struct vnode *nvp;
	struct buf *bp = NULL, *bp2 = NULL;
	union iso_ihead *ih;
	union iso_dhead *dh;
	int i, error, result;
	struct iso_mnt *imp;
	ino_t defino;
	
	ih = &iso_ihead[INOHASH(dev, ino)];
loop:
	for (ip = ih->ih_chain[0];
	     ip != (struct iso_node *)ih;
	     ip = ip->i_forw) {
		if (ino != ip->i_number || dev != ip->i_dev)
			continue;
		if ((ip->i_flag&ILOCKED) != 0) {
			ip->i_flag |= IWANT;
			sleep((caddr_t)ip, PINOD);
			goto loop;
		}
		if (vget(ITOV(ip)))
			goto loop;
		*ipp = ip;
		return(0);
	}
	/*
	 * Allocate a new inode.
	 */
	if (error = getnewvnode(VT_ISOFS, mntp, &isofs_vnodeops, &nvp)) {
		*ipp = 0;
		return (error);
	}
	ip = VTOI(nvp);
	ip->i_vnode = nvp;
	ip->i_flag = 0;
	ip->i_devvp = 0;
	ip->i_diroff = 0;
	ip->iso_parent = xp->i_diroff; /* Parent directory's */
	ip->iso_parent_ino = xp->i_number;
	ip->i_lockf = 0;
	
	/*
	 * Put it onto its hash chain and lock it so that other requests for
	 * this inode will block if they arrive while we are sleeping waiting
	 * for old data structures to be purged or for the contents of the
	 * disk portion of this inode to be read.
	 */
	ip->i_dev = dev;
	ip->i_number = ino;
	insque(ip, ih);
	ISO_ILOCK(ip);

	imp = VFSTOISOFS (mntp);
	ip->i_mnt = imp;
	ip->i_devvp = imp->im_devvp;
	VREF(ip->i_devvp);
	
	isofs_defino(isodir,&defino);
	if (defino != ino) {
	    /*
	     * This is a Rock Ridge relocated directory
	     * Read the `.' entry out of it.
	     */
	    if (error = iso_blkatoff(ip,0,&bp)) {
		vrele(ip->i_devvp);
		remque(ip);
		ip->i_forw = ip;
		ip->i_back = ip;
		iso_iput(ip);
		*ipp = 0;
		return error;
	    }
	    isodir = (struct iso_directory_record *)bp->b_un.b_addr;
	}
	
	ip->iso_extent = isonum_733 (isodir->extent);
	ip->i_size = isonum_733 (isodir->size);
	
	vp = ITOV(ip);
	
	/*
	 * Setup time stamp, attribute
	 */
	vp->v_type = VNON;
	switch ( imp->iso_ftype ) {
		default:	/* ISO_FTYPE_9660 */
			if ((imp->im_flags&ISOFSMNT_EXTATT)
			    && isonum_711(isodir->ext_attr_length))
				iso_blkatoff(ip,-isonum_711(isodir->ext_attr_length),
					     &bp2);
			isofs_defattr  ( isodir, ip, bp2 );
			isofs_deftstamp( isodir, ip, bp2 );
			break;  
		case ISO_FTYPE_RRIP:
			result = isofs_rrip_analyze( isodir, ip, imp );
			break;  
	}
	if (bp2)
		brelse(bp2);
	if (bp)
		brelse(bp);
	
	/*
	 * Initialize the associated vnode
	 */
	vp->v_type = IFTOVT(ip->inode.iso_mode);
	
	if ( vp->v_type == VFIFO ) {
#ifdef	FIFO
		extern struct vnodeops isofs_fifo_inodeops;
		vp->v_op = &isofs_fifo_inodeops;
#else
		iso_iput(ip);
		*ipp = 0;
		return (EOPNOTSUPP);
#endif	/* FIFO */
	} else if ( vp->v_type == VCHR || vp->v_type == VBLK ) {
		/*
		 * if device, look at device number table for translation
		 */
#ifdef	ISODEVMAP
		if (dp = iso_dmap(dev,ino,0))
			ip->inode.iso_rdev = dp->d_dev;
#endif
		vp->v_op = &isofs_spec_inodeops;
		if (nvp = checkalias(vp, ip->inode.iso_rdev, mntp)) {
			/*
			 * Reinitialize aliased inode.
			 */
			vp = nvp;
			iq = VTOI(vp);
			iq->i_vnode = vp;
			iq->i_flag = 0;
			ISO_ILOCK(iq);
			iq->i_dev = dev;
			iq->i_number = ino;
			iq->i_mnt = ip->i_mnt;
			bcopy(&ip->iso_extent,&iq->iso_extent,
			      (char *)(ip + 1) - (char *)ip->iso_extent);
			insque(iq, ih);
			/*
			 * Discard unneeded vnode
			 * (This introduces the need of INACTIVE modification)
			 */
			ip->inode.iso_mode = 0;
			iso_iput(ip);
			ip = iq;
		}
	}
	
	if (ip->iso_extent == imp->root_extent)
		vp->v_flag |= VROOT;
	
	*ipp = ip;
	return (0);
}

/*
 * Unlock and decrement the reference count of an inode structure.
 */
iso_iput(ip)
	register struct iso_node *ip;
{
	
	if ((ip->i_flag & ILOCKED) == 0)
		panic("iso_iput");
	ISO_IUNLOCK(ip);
	vrele(ITOV(ip));
}

/*
 * Last reference to an inode, write the inode out and if necessary,
 * truncate and deallocate the file.
 */
isofs_inactive(vp, p)
	struct vnode *vp;
	struct proc *p;
{
	register struct iso_node *ip = VTOI(vp);
	int mode, error = 0;
	
	if (prtactive && vp->v_usecount != 0)
		vprint("isofs_inactive: pushing active", vp);
	
	ip->i_flag = 0;
	/*
	 * If we are done with the inode, reclaim it
	 * so that it can be reused immediately.
	 */
	if (vp->v_usecount == 0 && ip->inode.iso_mode == 0)
		vgone(vp);
	return (error);
}

/*
 * Reclaim an inode so that it can be used for other purposes.
 */
isofs_reclaim(vp)
	register struct vnode *vp;
{
	register struct iso_node *ip = VTOI(vp);
	int i;
	
	if (prtactive && vp->v_usecount != 0)
		vprint("isofs_reclaim: pushing active", vp);
	/*
	 * Remove the inode from its hash chain.
	 */
	remque(ip);
	ip->i_forw = ip;
	ip->i_back = ip;
	/*
	 * Purge old data structures associated with the inode.
	 */
	cache_purge(vp);
	if (ip->i_devvp) {
		vrele(ip->i_devvp);
		ip->i_devvp = 0;
	}
	ip->i_flag = 0;
	return (0);
}

/*
 * Lock an inode. If its already locked, set the WANT bit and sleep.
 */
iso_ilock(ip)
	register struct iso_node *ip;
{
	
	while (ip->i_flag & ILOCKED) {
		ip->i_flag |= IWANT;
		if (ip->i_spare0 == curproc->p_pid)
			panic("locking against myself");
		ip->i_spare1 = curproc->p_pid;
		(void) sleep((caddr_t)ip, PINOD);
	}
	ip->i_spare1 = 0;
	ip->i_spare0 = curproc->p_pid;
	ip->i_flag |= ILOCKED;
}

/*
 * Unlock an inode.  If WANT bit is on, wakeup.
 */
iso_iunlock(ip)
	register struct iso_node *ip;
{

	if ((ip->i_flag & ILOCKED) == 0)
		vprint("iso_iunlock: unlocked inode", ITOV(ip));
	ip->i_spare0 = 0;
	ip->i_flag &= ~ILOCKED;
	if (ip->i_flag&IWANT) {
		ip->i_flag &= ~IWANT;
		wakeup((caddr_t)ip);
	}
}

/*
 * File attributes
 */
void isofs_defattr(isodir,inop,bp)
struct iso_directory_record *isodir;
struct iso_node *inop;
struct buf *bp;
{
    struct buf *bp2 = NULL;
    struct iso_mnt *imp;
    struct iso_extended_attributes *ap = NULL;
    int off;
    
    if (isonum_711(isodir->flags)&2) {
	inop->inode.iso_mode = S_IFDIR;
	inop->inode.iso_links = 2;
    } else {
	inop->inode.iso_mode = S_IFREG;
	inop->inode.iso_links = 1;
    }
    if (!bp
	&& ((imp = inop->i_mnt)->im_flags&ISOFSMNT_EXTATT)
	&& (off = isonum_711(isodir->ext_attr_length))) {
    	iso_blkatoff(inop,-off * imp->im_bsize,&bp2);
	bp = bp2;
    }
    if (bp) {
	ap = (struct iso_extended_attributes *)bp->b_un.b_addr;
	
	if (isonum_711(ap->version) == 1) {
	    if (!(ap->perm[0]&0x40))
		inop->inode.iso_mode |= VEXEC >> 6;
	    if (!(ap->perm[0]&0x10))
		inop->inode.iso_mode |= VREAD >> 6;
	    if (!(ap->perm[0]&4))
		inop->inode.iso_mode |= VEXEC >> 3;
	    if (!(ap->perm[0]&1))
		inop->inode.iso_mode |= VREAD >> 3;
	    if (!(ap->perm[1]&0x40))
		inop->inode.iso_mode |= VEXEC;
	    if (!(ap->perm[1]&0x10))
		inop->inode.iso_mode |= VREAD;
	    inop->inode.iso_uid = isonum_723(ap->owner); /* what about 0? */
	    inop->inode.iso_gid = isonum_723(ap->group); /* what about 0? */
	} else
	    ap = NULL;
    }
    if (!ap) {
	inop->inode.iso_mode |= VREAD|VEXEC|(VREAD|VEXEC)>>3|(VREAD|VEXEC)>>6;
	inop->inode.iso_uid = (uid_t)0;
	inop->inode.iso_gid = (gid_t)0;
    }
    if (bp2)
	brelse(bp2);
}

/*
 * Time stamps
 */
void isofs_deftstamp(isodir,inop,bp)
struct iso_directory_record *isodir;
struct iso_node *inop;
struct buf *bp;
{
    struct buf *bp2 = NULL;
    struct iso_mnt *imp;
    struct iso_extended_attributes *ap = NULL;
    int off;
    
    if (!bp
	&& ((imp = inop->i_mnt)->im_flags&ISOFSMNT_EXTATT)
	&& (off = isonum_711(isodir->ext_attr_length))) {
    	iso_blkatoff(inop,-off * imp->im_bsize,&bp2);
	bp = bp2;
    }
    if (bp) {
	ap = (struct iso_extended_attributes *)bp->b_un.b_addr;
	
	if (isonum_711(ap->version) == 1) {
	    if (!isofs_tstamp_conv17(ap->ftime,&inop->inode.iso_atime))
		isofs_tstamp_conv17(ap->ctime,&inop->inode.iso_atime);
	    if (!isofs_tstamp_conv17(ap->ctime,&inop->inode.iso_ctime))
		inop->inode.iso_ctime = inop->inode.iso_atime;
	    if (!isofs_tstamp_conv17(ap->mtime,&inop->inode.iso_mtime))
		inop->inode.iso_mtime = inop->inode.iso_ctime;
	} else
	    ap = NULL;
    }
    if (!ap) {
	isofs_tstamp_conv7(isodir->date,&inop->inode.iso_ctime);
	inop->inode.iso_atime = inop->inode.iso_ctime;
	inop->inode.iso_mtime = inop->inode.iso_ctime;
    }
    if (bp2)
	brelse(bp2);
}

int isofs_tstamp_conv7(pi,pu)
char *pi;
struct timeval *pu;
{
    int i;
    int crtime, days;
    int y, m, d, hour, minute, second, tz;
    
    y = pi[0] + 1900;
    m = pi[1];
    d = pi[2];
    hour = pi[3];
    minute = pi[4];
    second = pi[5];
    tz = pi[6];
    
    if (y < 1970) {
	pu->tv_sec  = 0;
	pu->tv_usec = 0;
	return 0;
    } else {
#ifdef	ORIGINAL
	/* computes day number relative to Sept. 19th,1989 */
	/* don't even *THINK* about changing formula. It works! */
	days = 367*(y-1980)-7*(y+(m+9)/12)/4-3*((y+(m-9)/7)/100+1)/4+275*m/9+d-100;
#else
	/*
	 * Changed :-) to make it relative to Jan. 1st, 1970
	 * and to disambiguate negative division
	 */
	days = 367*(y-1960)-7*(y+(m+9)/12)/4-3*((y+(m+9)/12-1)/100+1)/4+275*m/9+d-239;
#endif
	crtime = ((((days * 24) + hour) * 60 + minute) * 60) + second;
	
	/* timezone offset is unreliable on some disks */
	if (-48 <= tz && tz <= 52)
	    crtime += tz * 15 * 60;
    }
    pu->tv_sec  = crtime;
    pu->tv_usec = 0;
    return 1;
}

static unsigned isofs_chars2ui(begin,len)
unsigned char *begin;
int len;
{
    unsigned rc;
    
    for (rc = 0; --len >= 0;) {
	rc *= 10;
	rc += *begin++ - '0';
    }
    return rc;
}

int isofs_tstamp_conv17(pi,pu)
unsigned char *pi;
struct timeval *pu;
{
    unsigned char buf[7];
    
    /* year:"0001"-"9999" -> -1900  */
    buf[0] = isofs_chars2ui(pi,4) - 1900;
    
    /* month: " 1"-"12"      -> 1 - 12 */
    buf[1] = isofs_chars2ui(pi + 4,2);
    
    /* day:   " 1"-"31"      -> 1 - 31 */
    buf[2] = isofs_chars2ui(pi + 6,2);
    
    /* hour:  " 0"-"23"      -> 0 - 23 */
    buf[3] = isofs_chars2ui(pi + 8,2);
    
    /* minute:" 0"-"59"      -> 0 - 59 */
    buf[4] = isofs_chars2ui(pi + 10,2);
    
    /* second:" 0"-"59"      -> 0 - 59 */
    buf[5] = isofs_chars2ui(pi + 12,2);
    
    /* difference of GMT */
    buf[6] = pi[16];
    
    return isofs_tstamp_conv7(buf,pu);
}
