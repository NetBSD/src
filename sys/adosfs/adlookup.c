/*
 * Copyright (c) 1994 Christian E. Hopps
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
 *      This product includes software developed by Christian E. Hopps.
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
 *
 *	$Id: adlookup.c,v 1.4 1994/06/02 23:40:56 chopps Exp $
 */
#include <sys/param.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <adosfs/adosfs.h>

#define strmatch(s1, l1, s2, l2) \
    ((l1) == (l2) && bcmp((s1), (s2), (l1)) == 0)

/*
 * adosfs lookup. enters with:
 * pvp (parent vnode) referenced and locked.
 * exit with:
 *	target vp refernced and locked.
 *	unlock pvp unless LOCKPARENT and at end of search.
 * special cases:
 *	pvp == vp, just ref pvp, pvp already holds a ref and lock from
 *	    caller, this will not occur with RENAME or CREATE.
 *	LOOKUP always unlocks parent if last element. (not now!?!?)
 */
int
adosfs_lookup(pvp, ndp, p)
	struct vnode *pvp;
	struct nameidata *ndp;
	struct proc *p;
{
	int lockp, wantp, flag, error, last, cvalid, nocache, i;
	struct amount *amp;
	struct anode *pap, *ap;
	struct buf *bp;
	char *pelt;
	u_long bn, plen, hval;
#ifdef ADOSFS_DIAGNOSTIC
	printf("(adlookup ");
#endif
	ndp->ni_dvp = pvp;
	ndp->ni_vp = NULL;
	pap = VTOA(pvp);
	amp = pap->amp;
	lockp = ndp->ni_nameiop & LOCKPARENT;
	wantp = ndp->ni_nameiop & (LOCKPARENT | WANTPARENT);
	flag = ndp->ni_nameiop & OPMASK;
	pelt = ndp->ni_ptr;
	plen = ndp->ni_namelen;
	last = (*ndp->ni_next == 0);
	nocache = 0;
	
	/* 
	 * check that:
	 * pvp is a dir, and that the user has rights to access 
	 */
	if (pvp->v_type != VDIR) {
#ifdef ADOSFS_DIAGNOSTIC
		printf("ENOTDIR)");
#endif
		return(ENOTDIR);
	}
	if (error = VOP_ACCESS(pvp, VEXEC, ndp->ni_cred, p)) {
#ifdef ADOSFS_DIAGNOSTIC
		printf("[VOP_ACCESS] %d)", error);
#endif
		return(error);
	}
	/*
	 * cache lookup algorithm borrowed from ufs_lookup()
	 * its not consistent with otherthings in this function..
	 */
	if (error = cache_lookup(ndp)) {
		if (error == ENOENT) {
#ifdef ADOSFS_DIAGNOSTIC
			printf("[cache_lookup] %d)", error);
#endif
			return(error);
		}

		ap = VTOA(ndp->ni_vp);	
		cvalid = ndp->ni_vp->v_id;

		if (ap == pap) {
			VREF(pvp);
			error = 0;
		} else if (ndp->ni_isdotdot) {
			AUNLOCK(pap);	/* race */
			error = vget(ATOV(ap), 1);
			if (error == 0 && lockp && last)
				ALOCK(pap);
		} else {
			error = vget(ATOV(ap), 1);
			if (lockp == 0 || error || last)
				AUNLOCK(pap);
		}
		if (error == 0) {
			if (cvalid == ATOV(ap)->v_id) {
#ifdef ADOSFS_DIAGNOSTIC
				printf("[cache_lookup] 0)\n");
#endif
				return(0);
			}
			aput(ap);
			if (lockp && pap != ap && last)
				AUNLOCK(pap);
		}
		ALOCK(pap);
		ndp->ni_vp = NULL;
	}

	/*
	 * fake a '.'
	 */
	if (plen == 1 && pelt[0] == '.') {
		/* see special cases in prologue. */
		ap = pap;
		goto found;
	}
	/*
	 * fake a ".."
	 */
	if (ndp->ni_isdotdot) {
		if (pap->type == AROOT) 
			panic("adosfs .. attemped through root");
		/*
		 * cannot get previous entry while later is locked
		 * eg procA holds lock on previous and wats for pap
		 * we wait for previous and hold lock on pap. deadlock.
		 * becuase pap may have been acheived through symlink
		 * fancy detection code that decreases the race
		 * window size is not reasonably possible.
		 */
		AUNLOCK(pap); /* race */
		error = aget(amp->mp, pap->pblock, &ap);
		if (error || (last && lockp))
			ALOCK(pap);
		if (error) {
#ifdef ADOSFS_DIAGNOSTIC
			printf("[..] %d)", error);
#endif
			return(error);
		}
		goto found_lockdone;
	}

	/*
	 * hash the name and grab the first block in chain
	 * then walk the chain. if chain has not been fully
	 * walked before, track the count in `tabi'
	 */
	hval = adoshash(pelt, plen, pap->ntabent);
	bn = pap->tab[hval];
	i = min(pap->tabi[hval], 0);
	while (bn != 0) {
		if (error = aget(amp->mp, bn, &ap)) {
#ifdef ADOSFS_DIAGNOSTIC
			printf("[aget] %d)", error);
#endif
			return(error);
		}
		if (i <= 0) {
			if (--i < pap->tabi[hval])
				pap->tabi[hval] = i;	
			/*
			 * last header in chain lock count down by
			 * negating it to positive
			 */
			if (ap->hashf == 0) {
#ifdef DEBUG
				if (i != pap->tabi[hval])
					panic("adlookup: wrong chain count");
#endif
				pap->tabi[hval] = -pap->tabi[hval];
			}
		}
#ifdef ADOSFS_DIAGNOSTIC
		printf("%s =? %s", pelt, ap->name);
#endif
		if (strmatch(pelt, plen, ap->name, strlen(ap->name)))
			goto found;
		bn = ap->hashf;
		aput(ap);
	}
	/*
	 * not found
	 */
	if ((flag == CREATE || flag == RENAME) && last) {
		if (error = VOP_ACCESS(pvp, VWRITE, ndp->ni_cred, p)) {
#ifdef ADOSFS_DIAGNOSTIC
			printf("[VOP_ACCESS] %d)", error);
#endif
			return(error);
		}
		if (lockp == 0)
			AUNLOCK(pap);
		ndp->ni_nameiop |= SAVENAME;
#ifdef ADOSFS_DIAGNOSTIC
		printf("EJUSTRETURN)");
#endif
		return(EJUSTRETURN);
	}
	if (ndp->ni_makeentry && flag != CREATE)
		cache_enter(ndp);
#ifdef ADOSFS_DIAGNOSTIC
	printf("ENOENT)");
#endif
	return(ENOENT);

found:
	if (last && flag == DELETE)  {
		if (error = VOP_ACCESS(pvp, VWRITE, ndp->ni_cred, p)) {
			if (pap != ap)
				aput(ap);
#ifdef ADOSFS_DIAGNOSTIC
			printf("[VOP_ACCESS] %d)", error);
#endif
			return(error);
		}
		nocache = 1;
	} 
	if (last && flag == RENAME && wantp) {
		if (pap == ap) {
#ifdef ADOSFS_DIAGNOSTIC
			printf("EISDIR)");
#endif
			return(EISDIR);
		}
		if (error = VOP_ACCESS(pvp, VWRITE, ndp->ni_cred, p)) {
			aput(ap);
#ifdef ADOSFS_DIAGNOSTIC
			printf("[VOP_ACCESS] %d)", error);
#endif
			return(error);
		}
		ndp->ni_nameiop |= SAVENAME;
		nocache = 1;
	}
	if (ap == pap)
		VREF(pvp);
	else if (lockp == 0 || last == 0)
		AUNLOCK(pap);
found_lockdone:
	ndp->ni_vp = ATOV(ap);

	if (ndp->ni_makeentry && nocache == 0)
		cache_enter(ndp);
#ifdef ADOSFS_DIAGNOSTIC
	printf("0)\n");
#endif
	return(0);
}
