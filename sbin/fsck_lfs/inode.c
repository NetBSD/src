/*	$Id: inode.c,v 1.2.2.1 2000/01/21 00:34:56 he Exp $	*/

/*
 * Copyright (c) 1997, 1998
 *      Konrad Schroder.  All rights reserved.
 * Copyright (c) 1980, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 */

#include <sys/param.h>
#include <sys/time.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <sys/mount.h> /* XXX */
#include <ufs/lfs/lfs.h>
#ifndef SMALL
#include <pwd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fsck.h"
#include "fsutil.h"
#include "extern.h"

extern struct dinode **din_table;
extern SEGUSE *seg_table;

static int iblock __P((struct inodesc *, long, u_int64_t));
int blksreqd(struct lfs *, int);
int lfs_maxino(void);
SEGUSE *lfs_gseguse(int);
/* static void dump_inoblk __P((struct lfs *, struct dinode *)); */

/* stolen from lfs_inode.c */
/* Search a block for a specific dinode. */
struct dinode *
lfs_difind(struct lfs *fs, ino_t ino, struct dinode *dip)
{
    register int cnt;

    for(cnt=0;cnt<INOPB(fs);cnt++)
        if(dip[cnt].di_inumber == ino)
            return &(dip[cnt]);
    /* printf("lfs_difind: dinode %u not found\n", ino); */
    return NULL;
}

/*
 * Calculate the number of blocks required to be able to address data block
 * blkno (counting, of course, indirect blocks).  blkno must >=0.
 */
int blksreqd(struct lfs *fs, int blkno)
{
    long n = blkno;

    if(blkno < NDADDR)
        return blkno;
    n -= NDADDR;
    if(n < NINDIR(fs))
        return blkno + 1;
    n -= NINDIR(fs);
    if(n < NINDIR(fs)*NINDIR(fs))
        return blkno + 2 + n/NINDIR(fs) + 1;
    n -= NINDIR(fs)*NINDIR(fs);
    return blkno + 2 + NINDIR(fs) + n/(NINDIR(fs)*NINDIR(fs)) + 1;
}

#define BASE_SINDIR (NDADDR)
#define BASE_DINDIR (NDADDR+NINDIR(fs))
#define BASE_TINDIR (NDADDR+NINDIR(fs)+NINDIR(fs)*NINDIR(fs))

#define D_UNITS (NINDIR(fs))
#define T_UNITS (NINDIR(fs)*NINDIR(fs))

ufs_daddr_t lfs_bmap(struct lfs *, struct dinode *, ufs_daddr_t);

ufs_daddr_t lfs_bmap(struct lfs *fs, struct dinode *idinode, ufs_daddr_t lbn)
{
	ufs_daddr_t residue, up, off=0;
	struct bufarea *bp;
	
	if(lbn > 0 && lbn > (idinode->di_size-1)/dev_bsize) {
		return UNASSIGNED;
	}
	/*
	 * Indirect blocks: if it is a first-level indirect, pull its
	 * address from the inode; otherwise, call ourselves to find the
	 * address of the parent indirect block, and load that to find
	 * the desired address.
	 */
	if(lbn < 0) {
		lbn *= -1;
		if(lbn == NDADDR) {
			/* printf("lbn %d: single indir base\n", -lbn); */
			return idinode->di_ib[0]; /* single indirect */
		} else if(lbn == BASE_DINDIR+1) {
			/* printf("lbn %d: double indir base\n", -lbn); */
			return idinode->di_ib[1]; /* double indirect */
		} else if(lbn == BASE_TINDIR+2) {
			/* printf("lbn %d: triple indir base\n", -lbn); */
			return idinode->di_ib[2]; /* triple indirect */
		}

		/*
		 * Find the immediate parent. This is essentially finding the
		 * residue of modulus, and then rounding accordingly.
		 */
		residue = (lbn-NDADDR) % NINDIR(fs);
		if(residue == 1) {
			/* Double indirect.  Parent is the triple. */
			up = idinode->di_ib[2];
			off = (lbn-2-BASE_TINDIR)/(NINDIR(fs)*NINDIR(fs));
			if(up == UNASSIGNED || up == LFS_UNUSED_DADDR)
				return UNASSIGNED;
			/* printf("lbn %d: parent is the triple\n", -lbn); */
			bp = getddblk(up,sblock.lfs_bsize);
			bp->b_flags &= ~B_INUSE;
			return ((daddr_t *)(bp->b_un.b_buf))[off];
		} else /* residue == 0 */ {
			/* Single indirect.  Two cases. */
			if(lbn < BASE_TINDIR) {
				/* Parent is the double, simple */
				up = -(BASE_DINDIR) - 1;
				off = (lbn-BASE_DINDIR) / D_UNITS;
				/* printf("lbn %d: parent is %d/%d\n", -lbn, up,off); */
			} else {
				/* Ancestor is the triple, more complex */
				up = ((lbn-BASE_TINDIR) / T_UNITS)
					* T_UNITS + BASE_TINDIR + 1;
				off = (lbn/D_UNITS) - (up/D_UNITS);
				up = -up;
				/* printf("lbn %d: parent is %d/%d\n", -lbn, up,off); */
			}
		}
	} else {
		/* Direct block.  Its parent must be a single indirect. */
		if(lbn < NDADDR)
			return idinode->di_db[lbn];
		else {
			/* Parent is an indirect block. */
			up = -(((lbn-NDADDR) / D_UNITS) * D_UNITS + NDADDR);
			off = (lbn-NDADDR) % D_UNITS;
			/* printf("lbn %d: parent is %d/%d\n", lbn,up,off); */
		}
	}
	up = lfs_bmap(fs,idinode,up);
	if(up == UNASSIGNED || up == LFS_UNUSED_DADDR)
		return UNASSIGNED;
	bp = getddblk(up,sblock.lfs_bsize);
	bp->b_flags &= ~B_INUSE;
	return ((daddr_t *)(bp->b_un.b_buf))[off];
}

/*
 * This is kind of gross.  We use this to find the nth block
 * from a file whose inode has disk address idaddr.  In practice
 * we will only use this to find blocks of the ifile.
 */
struct bufarea *
getfileblk(struct lfs *fs, struct dinode *idinode, ino_t lbn)
{
    struct bufarea *bp;
    ufs_daddr_t blkno;
    static struct bufarea empty;
    static char empty_buf[65536];

    empty.b_un.b_buf = &(empty_buf[0]);

    blkno = lfs_bmap(fs,idinode,lbn);
    if(blkno == UNASSIGNED || blkno == LFS_UNUSED_DADDR) {
	    printf("Warning: ifile lbn %d unassigned!\n",lbn);
	    return &empty;
    }
    bp = getddblk(blkno,sblock.lfs_bsize);
    return bp;
}

int lfs_maxino(void)
{
#if 1
    struct dinode *idinode;

    idinode = lfs_difind(&sblock,sblock.lfs_ifile,&ifblock);
    return ((idinode->di_size
            - (sblock.lfs_cleansz + sblock.lfs_segtabsz) * sblock.lfs_bsize)
        / sblock.lfs_bsize) * sblock.lfs_ifpb - 1;
#else
    return sblock.lfs_nfiles;
#endif
}

static struct dinode *gidinode(void)
{
    static struct dinode *idinode;

    if(!idinode) {              /* only need to do this once */
        idinode = lfs_difind(&sblock,sblock.lfs_ifile,&ifblock);
        if(din_table[LFS_IFILE_INUM]
           && din_table[LFS_IFILE_INUM]->di_gen > idinode->di_gen) {
            printf("XXX replacing IFILE gen %d with gen %d\n",
                   idinode->di_gen, din_table[LFS_IFILE_INUM]->di_gen);
            idinode = din_table[LFS_IFILE_INUM];
        }
    }
    return idinode;
}

struct bufarea *
lfs_bginode(ino_t ino)
{
    ino_t blkno;

    /* this is almost verbatim from lfs.h LFS_IENTRY */
    blkno = ino/sblock.lfs_ifpb + sblock.lfs_cleansz + sblock.lfs_segtabsz;

    return getfileblk(&sblock,gidinode(),blkno);
}

struct ifile *
lfs_ientry(ino_t ino)
{
    struct ifile *ifp;
    struct bufarea *bp;

    bp = lfs_bginode(ino);
    if(bp)
    {
        ifp = (struct ifile *)malloc(sizeof(*ifp));
        *ifp = (((struct ifile *)(bp->b_un.b_buf))[ino%sblock.lfs_ifpb]);
        bp->b_flags &= ~B_INUSE;
        
        return ifp;
    }
    else
        return NULL;
}

SEGUSE *
lfs_gseguse(int segnum)
{
    int blkno;
    struct bufarea *bp;

    blkno = segnum/(sblock.lfs_bsize/sizeof(SEGUSE)) + sblock.lfs_cleansz;
    bp = getfileblk(&sblock,gidinode(),blkno);
    bp->b_flags &= ~B_INUSE;
    return ((SEGUSE *)bp->b_un.b_buf) + segnum%(sblock.lfs_bsize/sizeof(SEGUSE));
}

struct dinode *
lfs_ginode(ino_t inumber) {
    struct ifile *ifp;
    struct dinode *din;

    if (inumber == LFS_IFILE_INUM)
        return gidinode();
    if (/* inumber < ROOTINO || */ inumber > maxino)
        errexit("bad inode number %d to lfs_ginode\n", inumber);
    /* printf("[lfs_ginode: looking up inode %ld]\n",inumber); */
    ifp = lfs_ientry(inumber);
    if(ifp==NULL
       || ifp->if_daddr == LFS_UNUSED_DADDR
       || ifp->if_daddr == UNASSIGNED) {
        return NULL;
    }
    if(pbp)
        pbp->b_flags &= ~B_INUSE;
    if ( !(seg_table[datosn(&sblock,ifp->if_daddr)].su_flags & SEGUSE_DIRTY) )
    {
        printf("! INO %d: daddr 0x%x is in clean segment %d\n", inumber,
               ifp->if_daddr, datosn(&sblock,ifp->if_daddr));
    }
    pbp = getddblk(ifp->if_daddr,sblock.lfs_bsize);
    din=lfs_difind(&sblock, inumber, pbp->b_un.b_dinode);

    /* debug */
    if(din && din->di_inumber != inumber)
        printf("! lfs_ginode: got ino #%ld instead of %ld\n",
               (long)din->di_inumber, (long)inumber);
    free(ifp);
    return din;
}

/* imported from lfs_vfsops.c */
int
ino_to_fsba(struct lfs *fs, ino_t ino)
{
    daddr_t daddr = LFS_UNUSED_DADDR;
    struct ifile *ifp;

    /* Translate the inode number to a disk address. */
    if (ino == LFS_IFILE_INUM)
        daddr = fs->lfs_idaddr;
    else {
        /* LFS_IENTRY(ifp, fs, ino, bp); */
        ifp = lfs_ientry(ino);
        if(ifp) {
            daddr = ifp->if_daddr;
            free(ifp);
        } else {
            pwarn("Can't locate inode #%ud\n",ino);
        }
    }
    return daddr;
}

/*
 * Check validity of held (direct) blocks in an inode.
 * Note that this does not check held indirect blocks (although it does
 * check that the first level of indirection is valid).
 */
int
ckinode(dp, idesc)
    struct dinode *dp;
    register struct inodesc *idesc;
{
    register ufs_daddr_t *ap;
    long ret, n, ndb, offset;
    struct dinode dino;
    u_int64_t remsize, sizepb;
    mode_t mode;
    char pathbuf[MAXPATHLEN + 1];

    if (idesc->id_fix != IGNORE)
        idesc->id_fix = DONTKNOW;
    idesc->id_entryno = 0;
    idesc->id_filesize = dp->di_size;
    mode = dp->di_mode & IFMT;
    if (mode == IFBLK || mode == IFCHR
        || (mode == IFLNK &&
            (dp->di_size < sblock.lfs_maxsymlinklen ||
             (sblock.lfs_maxsymlinklen == 0 && dp->di_blocks == 0))))
        return (KEEPON);
    dino = *dp;
    ndb = howmany(dino.di_size, sblock.lfs_bsize);

    for (ap = &dino.di_db[0]; ap < &dino.di_db[NDADDR]; ap++) {
        if (--ndb == 0 && (offset = blkoff(&sblock, dino.di_size)) != 0) {
            idesc->id_numfrags =
                numfrags(&sblock, fragroundup(&sblock, offset));
        } else
            idesc->id_numfrags = sblock.lfs_frag;
        if (*ap == 0) {
            if (idesc->id_type == DATA && ndb >= 0) {
                /* An empty block in a directory XXX */
                getpathname(pathbuf, idesc->id_number,
                            idesc->id_number);
                pfatal("DIRECTORY %s: CONTAINS EMPTY BLOCKS",
                       pathbuf);
                if (reply("ADJUST LENGTH") == 1) {
                    dp = ginode(idesc->id_number);
                    dp->di_size = (ap - &dino.di_db[0]) *
                        sblock.lfs_bsize;
                    printf(
                           "YOU MUST RERUN FSCK AFTERWARDS\n");
                    rerun = 1;
                    inodirty();
                }
            }
            continue;
        }
        idesc->id_blkno = *ap;
        idesc->id_lblkno = ap - &dino.di_db[0];
        if (idesc->id_type == ADDR) {
            ret = (*idesc->id_func)(idesc);
        }
        else
            ret = dirscan(idesc);
	idesc->id_lblkno = 0;
        if (ret & STOP)
            return (ret);
    }
    idesc->id_numfrags = sblock.lfs_frag;
    remsize = dino.di_size - sblock.lfs_bsize * NDADDR;
    sizepb = sblock.lfs_bsize;
    for (ap = &dino.di_ib[0], n = 1; n <= NIADDR; ap++, n++) {
        if (*ap) {
            idesc->id_blkno = *ap;
            ret = iblock(idesc, n, remsize);
            if (ret & STOP)
                return (ret);
        } else {
            if (idesc->id_type == DATA && remsize > 0) {
                /* An empty block in a directory XXX */
                getpathname(pathbuf, idesc->id_number,
                            idesc->id_number);
                pfatal("DIRECTORY %s: CONTAINS EMPTY BLOCKS",
                       pathbuf);
                if (reply("ADJUST LENGTH") == 1) {
                    dp = ginode(idesc->id_number);
                    dp->di_size -= remsize;
                    remsize = 0;
                    printf(
                           "YOU MUST RERUN FSCK AFTERWARDS\n");
                    rerun = 1;
                    inodirty();
                    break;
                }
            }
        }
        sizepb *= NINDIR(&sblock);
        remsize -= sizepb;
    }
    return (KEEPON);
}

static int
iblock(idesc, ilevel, isize)
    struct inodesc *idesc;
    long ilevel;
    u_int64_t isize;
{
    register daddr_t *ap;
    register daddr_t *aplim;
    register struct bufarea *bp;
    int i, n, (*func) __P((struct inodesc *)), nif;
    u_int64_t sizepb;
    char pathbuf[MAXPATHLEN + 1], buf[BUFSIZ];
    struct dinode *dp;

    if (idesc->id_type == ADDR) {
        func = idesc->id_func;
        n = (*func)(idesc);
        if ((n & KEEPON) == 0)
            return (n);
    } else
        func = dirscan;
    if (chkrange(idesc->id_blkno, idesc->id_numfrags))
        return (SKIP);
    bp = getddblk(idesc->id_blkno, sblock.lfs_bsize);
    ilevel--;
    for (sizepb = sblock.lfs_bsize, i = 0; i < ilevel; i++)
        sizepb *= NINDIR(&sblock);
    if (isize > sizepb * NINDIR(&sblock))
        nif = NINDIR(&sblock);
    else
        nif = howmany(isize, sizepb);
    if (idesc->id_func == pass1check && nif < NINDIR(&sblock)) {
        aplim = &bp->b_un.b_indir[NINDIR(&sblock)];
        for (ap = &bp->b_un.b_indir[nif]; ap < aplim; ap++) {
            if (*ap == 0)
                continue;
            (void)sprintf(buf, "PARTIALLY TRUNCATED INODE I=%u",
                          idesc->id_number);
            if (dofix(idesc, buf)) {
                *ap = 0;
                dirty(bp);
            }
        }
        flush(fswritefd, bp);
    }
    aplim = &bp->b_un.b_indir[nif];
    for (ap = bp->b_un.b_indir; ap < aplim; ap++) {
        if (*ap) {
            idesc->id_blkno = *ap;
            if (ilevel == 0)
                n = (*func)(idesc);
            else
                n = iblock(idesc, ilevel, isize);
            if (n & STOP) {
                bp->b_flags &= ~B_INUSE;
                return (n);
            }
        } else {
            if (idesc->id_type == DATA && isize > 0) {
                /* An empty block in a directory XXX */
                getpathname(pathbuf, idesc->id_number,
                            idesc->id_number);
                pfatal("DIRECTORY %s: CONTAINS EMPTY BLOCKS",
                       pathbuf);
                if (reply("ADJUST LENGTH") == 1) {
                    dp = ginode(idesc->id_number);
                    dp->di_size -= isize;
                    isize = 0;
                    printf(
                           "YOU MUST RERUN FSCK AFTERWARDS\n");
                    rerun = 1;
                    inodirty();
                    bp->b_flags &= ~B_INUSE;
                    return(STOP);
                }
            }
        }
        isize -= sizepb;
    }
    bp->b_flags &= ~B_INUSE;
    return (KEEPON);
}

/*
 * Check that a block in a legal block number.
 * Return 0 if in range, 1 if out of range.
 */
int
chkrange(blk, cnt)
    daddr_t blk;
    int cnt;
{
    if (blk > fsbtodb(&sblock,maxfsblock)) {
        printf("daddr 0x%x too large\n", blk);
        return (1);
    }
    if ( !(seg_table[datosn(&sblock,blk)].su_flags & SEGUSE_DIRTY) ) {
        printf("daddr 0x%x is in clean segment 0x%x\n", blk, datosn(&sblock,blk));
        return (1);
    }
    return (0);
}

/*
 * General purpose interface for reading inodes.
 */
struct dinode *
ginode(ino_t inumber)
{
    return lfs_ginode(inumber);
}

/*
 * Routines to maintain information about directory inodes.
 * This is built during the first pass and used during the
 * second and third passes.
 *
 * Enter inodes into the cache.
 */
void
cacheino(dp, inumber)
    register struct dinode *dp;
    ino_t inumber;
{
    register struct inoinfo *inp;
    struct inoinfo **inpp;
    unsigned int blks;

    blks = howmany(dp->di_size, sblock.lfs_bsize);
    if (blks > NDADDR)
        blks = NDADDR + NIADDR;
    inp = (struct inoinfo *)
        malloc(sizeof(*inp) + (blks - 1) * sizeof(daddr_t));
    if (inp == NULL)
        return;
    inpp = &inphead[inumber % numdirs];
    inp->i_nexthash = *inpp;
    *inpp = inp;
    inp->i_child = inp->i_sibling = inp->i_parentp = 0;
    if (inumber == ROOTINO)
        inp->i_parent = ROOTINO;
    else
        inp->i_parent = (ino_t)0;
    inp->i_dotdot = (ino_t)0;
    inp->i_number = inumber;
    inp->i_isize = dp->di_size;
    inp->i_numblks = blks * sizeof(daddr_t);
    memcpy(&inp->i_blks[0], &dp->di_db[0], (size_t)inp->i_numblks);
    if (inplast == listmax) {
        listmax += 100;
        inpsort = (struct inoinfo **)realloc((char *)inpsort,
                                             (unsigned)listmax * sizeof(struct inoinfo *));
        if (inpsort == NULL)
            errexit("cannot increase directory list");
    }
    inpsort[inplast++] = inp;
}

/*
 * Look up an inode cache structure.
 */
struct inoinfo *
getinoinfo(inumber)
    ino_t inumber;
{
    register struct inoinfo *inp;

    for (inp = inphead[inumber % numdirs]; inp; inp = inp->i_nexthash) {
        if (inp->i_number != inumber)
            continue;
        return (inp);
    }
    errexit("cannot find inode %d\n", inumber);
    return ((struct inoinfo *)0);
}

/*
 * Clean up all the inode cache structure.
 */
void
inocleanup()
{
    register struct inoinfo **inpp;

    if (inphead == NULL)
        return;
    for (inpp = &inpsort[inplast - 1]; inpp >= inpsort; inpp--)
        free((char *)(*inpp));
    free((char *)inphead);
    free((char *)inpsort);
    inphead = inpsort = NULL;
}

void
inodirty()
{
	
    dirty(pbp);
}

void
clri(idesc, type, flag)
    register struct inodesc *idesc;
    char *type;
    int flag;
{
    register struct dinode *dp;

    dp = ginode(idesc->id_number);
    if (flag == 1) {
        pwarn("%s %s", type,
              (dp->di_mode & IFMT) == IFDIR ? "DIR" : "FILE");
        pinode(idesc->id_number);
    }
    if (preen || reply("CLEAR") == 1) {
        if (preen)
            printf(" (CLEARED)\n");
        n_files--;
        (void)ckinode(dp, idesc);
        clearinode(dp);
        statemap[idesc->id_number] = USTATE;
        inodirty();
    }
}

int
findname(idesc)
    struct inodesc *idesc;
{
    register struct direct *dirp = idesc->id_dirp;

    if (dirp->d_ino != idesc->id_parent)
        return (KEEPON);
    memcpy(idesc->id_name, dirp->d_name, (size_t)dirp->d_namlen + 1);
    return (STOP|FOUND);
}

int
findino(idesc)
    struct inodesc *idesc;
{
    register struct direct *dirp = idesc->id_dirp;

    if (dirp->d_ino == 0)
        return (KEEPON);
    if (strcmp(dirp->d_name, idesc->id_name) == 0 &&
        dirp->d_ino >= ROOTINO && dirp->d_ino <= maxino) {
        idesc->id_parent = dirp->d_ino;
        return (STOP|FOUND);
    }
    return (KEEPON);
}

void
pinode(ino)
    ino_t ino;
{
    register struct dinode *dp;
    register char *p;
    struct passwd *pw;
    time_t t;

    printf(" I=%u ", ino);
    if (ino < ROOTINO || ino > maxino)
        return;
    dp = ginode(ino);
    if(dp) {
        printf(" OWNER=");
#ifndef SMALL
        if ((pw = getpwuid((int)dp->di_uid)) != 0)
            printf("%s ", pw->pw_name);
        else
#endif
            printf("%u ", (unsigned)dp->di_uid);
        printf("MODE=%o\n", dp->di_mode);
        if (preen)
            printf("%s: ", cdevname());
        printf("SIZE=%qu ", (unsigned long long)dp->di_size);
        t = dp->di_mtime;
        p = ctime(&t);
        printf("MTIME=%12.12s %4.4s ", &p[4], &p[20]);
    }
}

void
blkerror(ino, type, blk)
    ino_t ino;
    char *type;
    daddr_t blk;
{

    pfatal("%d %s I=%u", blk, type, ino);
    printf("\n");
    if(exitonfail)
        exit(1);
    switch (statemap[ino]) {

      case FSTATE:
        statemap[ino] = FCLEAR;
        return;

      case DSTATE:
        statemap[ino] = DCLEAR;
        return;

      case FCLEAR:
      case DCLEAR:
        return;

      default:
        errexit("BAD STATE %d TO BLKERR", statemap[ino]);
        /* NOTREACHED */
    }
}

/*
 * allocate an unused inode
 */
ino_t
allocino(request, type)
    ino_t request;
    int type;
{
    register ino_t ino;
    register struct dinode *dp;
    time_t t;

    if (request == 0)
        request = ROOTINO;
    else if (statemap[request] != USTATE)
        return (0);
    for (ino = request; ino < maxino; ino++)
        if (statemap[ino] == USTATE)
            break;
    if (ino == maxino)
        return (0);
    switch (type & IFMT) {
      case IFDIR:
        statemap[ino] = DSTATE;
        break;
      case IFREG:
      case IFLNK:
        statemap[ino] = FSTATE;
        break;
      default:
        return (0);
    }
    dp = ginode(ino);
    dp->di_db[0] = allocblk((long)1);
    if (dp->di_db[0] == 0) {
        statemap[ino] = USTATE;
        return (0);
    }
    dp->di_mode = type;
    (void)time(&t);
    dp->di_atime = t;
    dp->di_mtime = dp->di_ctime = dp->di_atime;
    dp->di_size = sblock.lfs_fsize;
    dp->di_blocks = btodb(sblock.lfs_fsize);
    n_files++;
    inodirty();
    if (newinofmt)
        typemap[ino] = IFTODT(type);
    return (ino);
}

/*
 * deallocate an inode
 */
void
freeino(ino)
    ino_t ino;
{
    struct inodesc idesc;
    struct dinode *dp;

    memset(&idesc, 0, sizeof(struct inodesc));
    idesc.id_type = ADDR;
    idesc.id_func = pass4check;
    idesc.id_number = ino;
    dp = ginode(ino);
    (void)ckinode(dp, &idesc);
    clearinode(dp);
    inodirty();
    statemap[ino] = USTATE;
    n_files--;
}
