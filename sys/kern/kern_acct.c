/*
 * Copyright (c) 1994 Christopher G. Demetriou
 * Copyright (c) 1982, 1986, 1989 Regents of the University of California.
 * All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	from: @(#)kern_acct.c	7.18 (Berkeley) 5/11/91
 *	$Id: kern_acct.c,v 1.22 1994/05/17 04:21:49 cgd Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/resourcevar.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <sys/tty.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/acct.h>
#include <sys/syslog.h>

#include <vm/vm.h>
#include <vm/vm_param.h>

/*
 * Values associated with enabling and disabling accounting
 */
int	acctsuspend = 2;	/* stop accounting when < 2% free space left */
int	acctresume = 4;		/* resume when free space risen to > 4% */
struct	timeval chk = { 15, 0 };/* frequency to check space for accounting */
struct	timeval nextchk;	/* the next time space is checked */
struct  vnode *acctp = NULL;	/* file to which to do accounting */
struct  vnode *savacctp = NULL;	/* file to which to do accounting when space */

static comp_t encode_comp_t __P((u_long, u_long));

/*
 * Enable or disable process accounting.
 *
 * If a non-null filename is given, that file is used to store accounting
 * records on process exit. If a null filename is given process accounting
 * is suspended. If accounting is enabled, the system checks the amount
 * of freespace on the filesystem at timeval intervals. If the amount of
 * freespace is below acctsuspend percent, accounting is suspended. If
 * accounting has been suspended, and freespace rises above acctresume,
 * accounting is resumed.
 *
 * author: Mark Tinguely (tinguely@plains.NoDak.edu) 8/10/93
 */

struct acct_args {
	char *fname;
};
/* ARGSUSED */
acct(p, uap, retval)
	struct proc *p;
	struct acct_args *uap;
	int *retval;
{
	register struct nameidata *ndp;
	struct nameidata nd;
	struct vattr attr;
	int rv;
	void acctwatch();

        if (rv = suser(p->p_ucred, &p->p_acflag))	/* must be root */
                return(rv);

	/*
	 * Step 1. turn off accounting (if on). exit if fname is nil
	 */
	rv = 0;				/* just in case nothing is open */
	if (acctp != NULL) {
		rv = vn_close(acctp, FWRITE, p->p_ucred, p);
		/* turn off disk check */
		untimeout(acctwatch, &nextchk);
		acctp = NULL;
	}
	else if (savacctp != NULL ) {
		rv = vn_close(savacctp, FWRITE, p->p_ucred, p);
		/* turn off disk check */
		untimeout(acctwatch, &nextchk);
		savacctp = NULL;
	}
	if (uap->fname == NULL)		/* accounting stopping complete */
		return(rv);

	/*
	 * Step 2. open accounting filename for writing.
	 */
	nd.ni_segflg = UIO_USERSPACE;
	nd.ni_dirp = uap->fname;

	/* is it there? */
	if (rv = vn_open(&nd, p, FWRITE, 0))
		return (rv);

	/* Step 2. Check the attributes on accounting file */
	rv = VOP_GETATTR(nd.ni_vp, &attr, p->p_ucred, p);
	if (rv)
		goto acct_fail;

	/*
	 * is filesystem writable, do I have permission to write and is
	 * a regular file?
	 */
        if (nd.ni_vp->v_mount->mnt_flag & MNT_RDONLY) {
		rv = EROFS;	/* to be consistant with man page */
		goto acct_fail;
	}
	if ((VOP_ACCESS(nd.ni_vp, VWRITE, p->p_ucred, p)) ||
	    (attr.va_type != VREG)) {
		rv = EACCES;	/* permission denied error */
		goto acct_fail;
	}

	/* Step 3. Save the accounting file vnode, schedule freespace watch. */
	acctp  = nd.ni_vp;
	savacctp = NULL;
	VOP_UNLOCK(acctp);
	acctwatch(&nextchk);	/* look for full system */
	return(0);		/* end successfully */

acct_fail:
	vn_close(nd.ni_vp, FWRITE, p->p_ucred, p);
	return(rv);
}

/*
 * Periodically check the file system to see if accounting
 * should be turned on or off.
 */
void
acctwatch(resettime)
	struct timeval *resettime;
{
	struct statfs sb;
	int s;

	if (savacctp) {	/* accounting suspended */
		(void)VFS_STATFS(savacctp->v_mount, &sb, (struct proc *)0);
		if (sb.f_bavail > acctresume * sb.f_blocks / 100) {
			acctp = savacctp;
			savacctp = NULL;
			log(LOG_NOTICE, "Accounting resumed\n");
		}
	} else if (acctp) { /* accounting going on */
		(void)VFS_STATFS(acctp->v_mount, &sb, (struct proc *)0);
		if (sb.f_bavail <= acctsuspend * sb.f_blocks / 100) {
			savacctp = acctp;
			acctp = NULL;
			log(LOG_NOTICE, "Accounting suspended\n");
		}
	} else /* accounting yanked out from under us */
		return;

	s = splhigh(); *resettime = time; splx(s);
	timevaladd(resettime, &chk);
	timeout(acctwatch, resettime, hzto(resettime));
}

/*
 * This routine calculates an accounting record for a process and,
 * if accounting is enabled, writes it to the accounting file.
 *
 * author: Mark Tinguely (tinguely@plains.NoDak.edu) 8/10/93
 */

acct_process(p)
	register struct proc *p;
{
	struct acct acct;
	struct rusage *r;
	struct timeval tmptv, ut, st;
	int rv;
	long i;
	u_int cnt;
	char *c;

	if (acctp == NULL)	/* accounting not turned on */
		return;

	/*
	 * get process accounting information
	 */
	bcopy(p->p_comm, acct.ac_comm, sizeof acct.ac_comm);
	calcru(p, &ut, &st, NULL);
	acct.ac_utime = encode_comp_t(ut.tv_sec, ut.tv_usec);
	acct.ac_stime = encode_comp_t(st.tv_sec, st.tv_usec);
	acct.ac_btime = p->p_stats->p_start.tv_sec;
	tmptv = time;
	timevalsub(&tmptv, &p->p_stats->p_start);
	acct.ac_etime = encode_comp_t(tmptv.tv_sec, tmptv.tv_usec);
	acct.ac_uid = p->p_cred->p_ruid;
	acct.ac_gid = p->p_cred->p_rgid;

	r = &p->p_stats->p_ru;
	tmptv = ut;
	timevaladd(&tmptv, &st);
	i = (tmptv.tv_sec * hz) + (tmptv.tv_usec / tick);
	if (i)
		acct.ac_mem = (r->ru_ixrss + r->ru_idrss + r->ru_isrss) / i;
	else
		acct.ac_mem = 0;
	acct.ac_io = encode_comp_t(r->ru_inblock + r->ru_oublock, 0);

	if ((p->p_flag & P_CONTROLT) && p->p_pgrp->pg_session->s_ttyp)
		acct.ac_tty = p->p_pgrp->pg_session->s_ttyp->t_dev;
	else
		acct.ac_tty = NODEV;
	acct.ac_flag = p->p_acflag;

	/*
	 * wirte accounting record to the file
	 */
	rv = vn_rdwr(UIO_WRITE, acctp, (caddr_t) &acct, sizeof (acct), 
	    (off_t)0, UIO_SYSSPACE, IO_APPEND|IO_UNIT, p->p_ucred,
	    (int *) NULL, p);
}

/*
 * encode_comp_t converts from ticks in seconds and microseconds to ticks
 * in 1/AHZ seconds
 * 
 * comp_t is a psuedo-floating point number with 13 bits of
 * mantissa and 3 bits of base 8 exponent and has resolution
 * of 1/AHZ seconds.
 */

#define MANT		13			/* 13-bit mantissa */
#define EXP		3			/* base 8 exponent */
#define MAXFRACT	((1 << MANT) - 1)	/* max fract value */

static comp_t
encode_comp_t(s, us)
	u_long s, us;
{
	int exp, rnd;

	exp = 0;
	rnd = 0;
	s *= AHZ;
	s += us / (1000000 / AHZ);		/* maximize precision */

	while (s > MAXFRACT) {
		rnd = s & (1 << (EXP-1));	/* round up? */
		s >>= EXP;			/* base 8 exponent */
		exp++;
	}

	/* if we need to round up, do it (and handle overflow correctly) */
	if (rnd && (++s > MAXFRACT)) {
		s >>= EXP;
		exp++;
	}

	exp <<= MANT;			/* move the exponent */
	exp += s;			/* add on the mantissa */
	return (exp);
}
