/*-
 * Copyright (c) 1993 Christopher G. Demetriou
 * Copyright (c) 1989 The Regents of the University of California.
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
 */

#if defined(LIBC_SCCS) && !defined(lint)
/*static char sccsid[] = "from: @(#)kvm.c	5.18 (Berkeley) 5/7/91";*/
static char rcsid[] = "$Id: kvm.c,v 1.23 1994/01/07 19:10:06 cgd Exp $";
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/kinfo.h>
#include <sys/tty.h>
#include <sys/exec.h>
#include <machine/vmparam.h>
#include <fcntl.h>
#include <nlist.h>
#include <kvm.h>
#include <ndbm.h>
#include <limits.h>
#include <paths.h>
#include <stdio.h>
#include <string.h>

#define	btop(x)		(((unsigned)(x)) >> PGSHIFT)	/* XXX */
#define	ptob(x)		((caddr_t)((x) << PGSHIFT))	/* XXX */
#include <vm/vm.h>	/* ??? kinfo_proc currently includes this*/
#include <vm/vm_page.h>
#include <vm/swap_pager.h>
#include <sys/kinfo_proc.h>
#if defined(m68k)
#include <machine/pte.h>
#define	btos(x)		(((unsigned)(x)) >> SEGSHIFT)	/* XXX */
#endif

/*
 * files
 */
static	const char *unixf, *memf, *kmemf, *swapf;
static	int unixx, mem, kmem, swap;
static	DBM *db;
/*
 * flags
 */
static	int deadkernel;
static	int kvminit = 0;
static	int kvmfilesopen = 0;
/*
 * state
 */
static	struct kinfo_proc *kvmprocbase, *kvmprocptr;
static	int kvmnprocs;
/*
 * u. buffer
 */
static union {
	struct	user user;
	char	upages[UPAGES][NBPG];
} user;

struct swapblk {
	long	offset;		/* offset in swap device */
	long	size;		/* remaining size of block in swap device */
};

/*
 * random other stuff
 */
static	int	dmmin, dmmax;
static	int	pcbpf;
static	int	nswap;
static	long	vm_page_hash_mask;
static	long	vm_page_buckets;
static	long	page_shift;
static	char	*tmp;
#if defined(m68k)
static	int	lowram;
static	struct ste *Sysseg;
#endif
#if defined(i386)
static	struct pde *PTD;
#endif

#define atop(x)		(((unsigned)(x)) >> page_shift)
#define vm_page_hash(object, offset) \
        (((unsigned)object+(unsigned)atop(offset))&vm_page_hash_mask)

#define basename(cp)	((tmp=rindex((cp), '/')) ? tmp+1 : (cp))
#define	MAXSYMSIZE	256

static struct nlist nl[] = {
	{ "_Usrptmap" },
#define	X_USRPTMAP	0
	{ "_usrpt" },
#define	X_USRPT		1
	{ "_nswap" },
#define	X_NSWAP		2
	{ "_dmmin" },
#define	X_DMMIN		3
	{ "_dmmax" },
#define	X_DMMAX		4
	{ "_vm_page_buckets" },
#define X_VM_PAGE_BUCKETS	5
	{ "_vm_page_hash_mask" },
#define X_VM_PAGE_HASH_MASK	6
	{ "_page_shift" },
#define X_PAGE_SHIFT	7
	/*
	 * everything here and down, only if a dead kernel
	 */
	{ "_Sysmap" },
#define	X_SYSMAP	8
#define	X_DEADKERNEL	X_SYSMAP
	{ "_Syssize" },
#define	X_SYSSIZE	9
	{ "_allproc" },
#define X_ALLPROC	10
	{ "_zombproc" },
#define X_ZOMBPROC	11
	{ "_nproc" },
#define	X_NPROC		12
#define	X_LAST		12
#if defined(m68k)
	{ "_Sysseg" },
#define	X_SYSSEG	(X_LAST+1)
	{ "_lowram" },
#define	X_LOWRAM	(X_LAST+2)
#endif
#if defined(i386)
	{ "_IdlePTD" },
#define	X_IdlePTD	(X_LAST+1)
#endif
	{ "" },
};

static off_t Vtophys();
static void klseek(), seterr(), setsyserr(), vstodb();
static int getkvars(), kvm_doprocs(), kvm_init();
static int vatosw();
static int pager_get();
static int findpage();

/*
 * returns 	0 if files were opened now,
 * 		1 if files were already opened,
 *		-1 if files could not be opened.
 */
kvm_openfiles(uf, mf, sf)
	const char *uf, *mf, *sf; 
{
	if (kvmfilesopen)
		return (1);
	unixx = mem = kmem = swap = -1;
	unixf = (uf == NULL) ? _PATH_UNIX : uf; 
	memf = (mf == NULL) ? _PATH_MEM : mf;

	if ((unixx = open(unixf, O_RDONLY, 0)) == -1) {
		setsyserr("can't open %s", unixf);
		goto failed;
	}
	if ((mem = open(memf, O_RDONLY, 0)) == -1) {
		setsyserr("can't open %s", memf);
		goto failed;
	}
	if (sf != NULL)
		swapf = sf;
	if (mf != NULL) {
		deadkernel++;
		kmemf = mf;
		kmem = mem;
		swap = -1;
	} else {
		kmemf = _PATH_KMEM;
		if ((kmem = open(kmemf, O_RDONLY, 0)) == -1) {
			setsyserr("can't open %s", kmemf);
			goto failed;
		}
		swapf = (sf == NULL) ?  _PATH_DRUM : sf;
		/*
		 * live kernel - avoid looking up nlist entries
		 * past X_DEADKERNEL.
		 */
		nl[X_DEADKERNEL].n_name = "";
	}
	if (swapf != NULL && ((swap = open(swapf, O_RDONLY, 0)) == -1)) {
		seterr("can't open %s", swapf);
		goto failed;
	}
	kvmfilesopen++;
	if (kvminit == 0 && kvm_init(NULL, NULL, NULL, 0) == -1) /*XXX*/
		return (-1);
	return (0);
failed:
	kvm_close();
	return (-1);
}

static
kvm_init(uf, mf, sf)
	char *uf, *mf, *sf;
{
	if (kvmfilesopen == 0 && kvm_openfiles(NULL, NULL, NULL) == -1)
		return (-1);
	if (getkvars() == -1)
		return (-1);
	kvminit = 1;

	return (0);
}

kvm_close()
{
	if (unixx != -1) {
		close(unixx);
		unixx = -1;
	}
	if (kmem != -1) {
		if (kmem != mem)
			close(kmem);
		/* otherwise kmem is a copy of mem, and will be closed below */
		kmem = -1;
	}
	if (mem != -1) {
		close(mem);
		mem = -1;
	}
	if (swap != -1) {
		close(swap);
		swap = -1;
	}
	if (db != NULL) {
		dbm_close(db);
		db = NULL;
	}
	kvminit = 0;
	kvmfilesopen = 0;
	deadkernel = 0;
}

kvm_nlist(nl)
	struct nlist *nl;
{
	datum key, data;
	char dbname[MAXPATHLEN];
	char dbversion[_POSIX2_LINE_MAX];
	char kversion[_POSIX2_LINE_MAX];
	int dbversionlen;
	char symbuf[MAXSYMSIZE];
	struct nlist nbuf, *n;
	int num, did;

	if (kvmfilesopen == 0 && kvm_openfiles(NULL, NULL, NULL) == -1)
		return (-1);
	if (deadkernel)
		goto hard2;
	/*
	 * initialize key datum
	 */
	key.dptr = symbuf;

	if (db != NULL)
		goto win;	/* off to the races */
	/*
	 * open database
	 */
	sprintf(dbname, "%s/kvm_%s", _PATH_VARRUN, basename(unixf));
	if ((db = dbm_open(dbname, O_RDONLY, 0)) == NULL)
		goto hard2;
	/*
	 * read version out of database
	 */
	bcopy("VERSION", symbuf, sizeof ("VERSION")-1);
	key.dsize = (sizeof ("VERSION") - 1);
	data = dbm_fetch(db, key);
	if (data.dptr == NULL)
		goto hard1;
	bcopy(data.dptr, dbversion, data.dsize);
	dbversionlen = data.dsize;
	/*
	 * read version string from kernel memory
	 */
	bcopy("_version", symbuf, sizeof ("_version")-1);
	key.dsize = (sizeof ("_version")-1);
	data = dbm_fetch(db, key);
	if (data.dptr == NULL)
		goto hard1;
	if (data.dsize != sizeof (struct nlist))
		goto hard1;
	bcopy(data.dptr, &nbuf, sizeof (struct nlist));
	lseek(kmem, nbuf.n_value, 0);
	if (read(kmem, kversion, dbversionlen) != dbversionlen)
		goto hard1;
	/*
	 * if they match, we win - otherwise do it the hard way
	 */
	if (bcmp(dbversion, kversion, dbversionlen) != 0)
		goto hard1;
	/*
	 * getem from the database.
	 */
win:
	num = did = 0;
	for (n = nl; n->n_name && n->n_name[0]; n++, num++) {
		int len;
		/*
		 * clear out fields from users buffer
		 */
		n->n_type = 0;
		n->n_other = 0;
		n->n_desc = 0;
		n->n_value = 0;
		/*
		 * query db
		 */
		if ((len = strlen(n->n_name)) > MAXSYMSIZE) {
			seterr("symbol too large");
			return (-1);
		}
		(void)strcpy(symbuf, n->n_name);
		key.dsize = len;
		data = dbm_fetch(db, key);
		if (data.dptr == NULL || data.dsize != sizeof (struct nlist))
			continue;
		bcopy(data.dptr, &nbuf, sizeof (struct nlist));
		n->n_value = nbuf.n_value;
		n->n_type = nbuf.n_type;
		n->n_desc = nbuf.n_desc;
		n->n_other = nbuf.n_other;
		did++;
	}
	return (num - did);
hard1:
	dbm_close(db);
	db = NULL;
hard2:
	num = nlist(unixf, nl);
	if (num == -1)
		seterr("nlist (hard way) failed");
	return (num);
}

kvm_getprocs(what, arg)
	int what, arg;
{
	static int	ocopysize = -1;

	if (kvminit == 0 && kvm_init(NULL, NULL, NULL, 0) == -1)
		return (NULL);
	if (!deadkernel) {
		int ret, copysize;

		if ((ret = getkerninfo(what, NULL, NULL, arg)) == -1) {
			setsyserr("can't get estimate for kerninfo");
			return (-1);
		}
		copysize = ret;
		if (copysize > ocopysize || !kvmprocbase) {
			if (ocopysize == -1 || !kvmprocbase)
				kvmprocbase =
					(struct kinfo_proc *)malloc(copysize);
			else
				kvmprocbase =
					(struct kinfo_proc *)realloc(kvmprocbase,
								copysize);
			if (!kvmprocbase) {
				seterr("out of memory");
				return (-1);
			}
		}
		ocopysize = copysize;
		if ((ret = getkerninfo(what, kvmprocbase, &copysize, 
		     arg)) == -1) {
			setsyserr("can't get proc list");
			return (-1);
		}
		if (copysize % sizeof (struct kinfo_proc)) {
			seterr("proc size mismatch (got %d total, kinfo_proc: %d)",
				copysize, sizeof (struct kinfo_proc));
			return (-1);
		}
		kvmnprocs = copysize / sizeof (struct kinfo_proc);
	} else {
		int nproc;

		if (kvm_read((void *) nl[X_NPROC].n_value, &nproc,
		    sizeof (int)) == -1) {
			seterr("can't read nproc");
			return (-1);
		}
		if ((kvmprocbase = (struct kinfo_proc *)
		     malloc(nproc * sizeof (struct kinfo_proc))) == NULL) {
			seterr("out of memory (addr: %x nproc = %d)",
				nl[X_NPROC].n_value, nproc);
			return (-1);
		}
		kvmnprocs = kvm_doprocs(what, arg, kvmprocbase);
		realloc(kvmprocbase, kvmnprocs * sizeof (struct kinfo_proc));
	}
	kvmprocptr = kvmprocbase;

	return (kvmnprocs);
}

/*
 * XXX - should NOT give up so easily - especially since the kernel
 * may be corrupt (it died).  Should gather as much information as possible.
 * Follows proc ptrs instead of reading table since table may go
 * away soon.
 */
static
kvm_doprocs(what, arg, buff)
	int what, arg;
	char *buff;
{
	struct proc *p, proc;
	register char *bp = buff;
	int i = 0;
	int doingzomb = 0;
	struct eproc eproc;
	struct pgrp pgrp;
	struct session sess;
	struct tty tty;

	/* allproc */
	if (kvm_read((void *) nl[X_ALLPROC].n_value, &p, 
	    sizeof (struct proc *)) == -1) {
		seterr("can't read allproc");
		return (-1);
	}

again:
	for (; p; p = proc.p_nxt) {
		if (kvm_read(p, &proc, sizeof (struct proc)) == -1) {
			seterr("can't read proc at %x", p);
			return (-1);
		}
		if (kvm_read(proc.p_cred, &eproc.e_pcred,
		    sizeof (struct pcred)) != -1)
			(void) kvm_read(eproc.e_pcred.pc_ucred, &eproc.e_ucred,
			    sizeof (struct ucred));

		switch(ki_op(what)) {
			
		case KINFO_PROC_PID:
			if (proc.p_pid != (pid_t)arg)
				continue;
			break;


		case KINFO_PROC_UID:
			if (eproc.e_ucred.cr_uid != (uid_t)arg)
				continue;
			break;

		case KINFO_PROC_RUID:
			if (eproc.e_pcred.p_ruid != (uid_t)arg)
				continue;
			break;
		}
		/*
		 * gather eproc
		 */
		eproc.e_paddr = p;
		if (kvm_read(proc.p_pgrp, &pgrp, sizeof (struct pgrp)) == -1) {
			seterr("can't read pgrp at %x", proc.p_pgrp);
			return (-1);
		}
		eproc.e_sess = pgrp.pg_session;
		eproc.e_pgid = pgrp.pg_id;
		eproc.e_jobc = pgrp.pg_jobc;
		if (kvm_read(pgrp.pg_session, &sess, sizeof (struct session))
		    == -1) {
			seterr("can't read session at %x", pgrp.pg_session);
			return (-1);
		}
		if ((proc.p_flag&SCTTY) && sess.s_ttyp != NULL) {
			if (kvm_read(sess.s_ttyp, &tty, sizeof (struct tty))
			    == -1) {
				seterr("can't read tty at %x", sess.s_ttyp);
				return (-1);
			}
			eproc.e_tdev = tty.t_dev;
			eproc.e_tsess = tty.t_session;
			if (tty.t_pgrp != NULL) {
				if (kvm_read(tty.t_pgrp, &pgrp, sizeof (struct
				    pgrp)) == -1) {
					seterr("can't read tpgrp at &x", 
						tty.t_pgrp);
					return (-1);
				}
				eproc.e_tpgid = pgrp.pg_id;
			} else
				eproc.e_tpgid = -1;
		} else
			eproc.e_tdev = NODEV;
		if (proc.p_wmesg)
			(void) kvm_read(proc.p_wmesg, eproc.e_wmesg, WMESGLEN);
		(void) kvm_read(proc.p_vmspace, &eproc.e_vm,
		    sizeof (struct vmspace));
		eproc.e_xsize = eproc.e_xrssize =
			eproc.e_xccount = eproc.e_xswrss = 0;

		switch(ki_op(what)) {

		case KINFO_PROC_PGRP:
			if (eproc.e_pgid != (pid_t)arg)
				continue;
			break;

		case KINFO_PROC_TTY:
			if ((proc.p_flag&SCTTY) == 0 || 
			     eproc.e_tdev != (dev_t)arg)
				continue;
			break;
		}

		i++;
		bcopy(&proc, bp, sizeof (struct proc));
		bp += sizeof (struct proc);
		bcopy(&eproc, bp, sizeof (struct eproc));
		bp+= sizeof (struct eproc);
	}
	if (!doingzomb) {
		/* zombproc */
		if (kvm_read((void *) nl[X_ZOMBPROC].n_value, &p, 
		    sizeof (struct proc *)) == -1) {
			seterr("can't read zombproc");
			return (-1);
		}
		doingzomb = 1;
		goto again;
	}

	return (i);
}

struct proc *
kvm_nextproc()
{

	if (!kvmprocbase && kvm_getprocs(0, 0) == -1)
		return (NULL);
	if (kvmprocptr >= (kvmprocbase + kvmnprocs)) {
		seterr("end of proc list");
		return (NULL);
	}
	return((struct proc *)(kvmprocptr++));
}

struct eproc *
kvm_geteproc(p)
	const struct proc *p;
{
	return ((struct eproc *)(((char *)p) + sizeof (struct proc)));
}

kvm_setproc()
{
	kvmprocptr = kvmprocbase;
}

kvm_freeprocs()
{

	if (kvmprocbase) {
		free(kvmprocbase);
		kvmprocbase = NULL;
	}
}

struct user *
kvm_getu(p)
	const struct proc *p;
{
	register struct kinfo_proc *kp = (struct kinfo_proc *)p;
	register int i;
	register char *up;
	u_int vaddr;
	struct swapblk swb;

	if (kvminit == 0 && kvm_init(NULL, NULL, NULL, 0) == -1)
		return (NULL);
	if (p->p_stat == SZOMB) {
		seterr("zombie process");
		return (NULL);
	}

	if ((p->p_flag & SLOAD) == 0) {
		vm_offset_t	maddr;

		if (swap < 0) {
			seterr("no swap");
			return (NULL);
		}
		/*
		 * Costly operation, better set enable_swap to zero
		 * in vm/vm_glue.c, since paging of user pages isn't
		 * done yet anyway.
		 */
		if (vatosw(&kp->kp_eproc.e_vm.vm_map, USRSTACK + i * NBPG,
			   &maddr, &swb) == 0)
			return NULL;

		if (maddr == 0 && swb.size < UPAGES * NBPG)
			return NULL;

		for (i = 0; i < UPAGES; i++) {
			if (maddr) {
				(void) lseek(mem, maddr + i * NBPG, 0);
				if (read(mem,
				    (char *)user.upages[i], NBPG) != NBPG) {
					seterr(
					    "can't read u for pid %d from %s",
					    p->p_pid, swapf);
					return NULL;
				}
			} else {
				(void) lseek(swap, swb.offset + i * NBPG, 0);
				if (read(swap,
				    (char *)user.upages[i], NBPG) != NBPG) {
					seterr(
					    "can't read u for pid %d from %s",
					    p->p_pid, swapf);
					return NULL;
				}
			}
		}
		return(&user.user);
	}
	/*
	 * Read u-area one page at a time for the benefit of post-mortems
	 */
	up = (char *) p->p_addr;
	for (i = 0; i < UPAGES; i++) {
		klseek(kmem, (long)up, 0);
		if (read(kmem, user.upages[i], CLBYTES) != CLBYTES) {
			seterr("cant read page %x of u of pid %d from %s",
			    up, p->p_pid, kmemf);
			return(NULL);
		}
		up += CLBYTES;
	}
	pcbpf = (int) btop(p->p_addr);	/* what should this be really? */

	return(&user.user);
}

int
kvm_procread(p, addr, buf, len)
	const struct proc *p;
	const unsigned addr;
	unsigned len;
	char *buf;
{
	register struct kinfo_proc *kp = (struct kinfo_proc *) p;
	struct swapblk swb;
	vm_offset_t swaddr = 0, memaddr = 0;
	unsigned real_len;

	real_len = len < (CLBYTES - (addr & CLOFSET)) ? len : (CLBYTES - (addr & CLOFSET));

        if (vatosw(&kp->kp_eproc.e_vm.vm_map, addr & ~CLOFSET, &memaddr,
		   &swb) == 0)
		return 0;

	if (memaddr) {
		memaddr += addr & CLOFSET;
		if (lseek(mem, memaddr, 0) == -1)
			seterr("kvm_procread: lseek mem");
		len = read(mem, buf, real_len);
		if (len == -1) {
			seterr("kvm_procread: read mem");
			return 0;
		}
	} else {
		char bouncebuf[CLBYTES];
		swaddr = swb.offset + (addr & CLOFSET);
		swb.size -= addr & CLOFSET;
		if (lseek(swap, swaddr & ~CLOFSET, 0) == -1) {
			seterr("kvm_procread: lseek swap");
			return 0;
		}
		len = read(swap, bouncebuf, CLBYTES);
		if (len == -1 || len <= (swaddr & CLOFSET)) {
			seterr("kvm_procread: read swap");
			return 0;
		}
		len = MIN(len - (swaddr & CLOFSET), real_len);
		memcpy(buf, &bouncebuf[swaddr & CLOFSET], len);
	}

	return len;
}

int
kvm_procreadstr(p, addr, buf, len)
        const struct proc *p;
        const unsigned addr;
	char *buf;
	unsigned len;
{
	int	done, little;
	char	copy[200], *pb;
	char	a;

	done = 0;
	copy[0] = '\0';
	while (len) {
		little = kvm_procread(p, addr+done, copy, MIN(len, sizeof copy));
		if (little<1)
			break;
		pb = copy;
		while (little--) {
			len--;
			if( (*buf++ = *pb++) == '\0' )
				return done;
			done++;
		}
	}
	return done;
}

char *
kvm_getargs(p, up)
	const struct proc *p;
	const struct user *up;
{
	static char cmdbuf[ARG_MAX + sizeof(p->p_comm) + 5];
	register char *cp, *acp;
	int left, rv;
	struct ps_strings arginfo;

	if (up == NULL || p->p_pid == 0 || p->p_pid == 2)
		goto retucomm;

	if (kvm_procread(p, PS_STRINGS, (char *)&arginfo, sizeof(arginfo)) !=
		sizeof(arginfo))
		goto bad;

	cmdbuf[0] = '\0';
	cp = cmdbuf;
	acp = arginfo.ps_argvstr;
	left = ARG_MAX + 1;
	while (arginfo.ps_nargvstr--) {
		if ((rv = kvm_procreadstr(p, acp, cp, left)) >= 0) {
			acp += rv + 1;
			left -= rv + 1;
			cp += rv;
			*cp++ = ' ';
			*cp = '\0';
		} else
			goto bad;
	}
	cp-- ; *cp = '\0';

	if (cmdbuf[0] == '-' || cmdbuf[0] == '?' || cmdbuf[0] <= ' ') {
		(void) strcat(cmdbuf, " (");
		(void) strncat(cmdbuf, p->p_comm, sizeof(p->p_comm));
		(void) strcat(cmdbuf, ")");
	}
	return (cmdbuf);

bad:
	seterr("error locating command name for pid %d", p->p_pid);
retucomm:
	(void) strcpy(cmdbuf, "(");
	(void) strncat(cmdbuf, p->p_comm, sizeof (p->p_comm));
	(void) strcat(cmdbuf, ")");
	return (cmdbuf);
}

char *
kvm_getenv(p, up)
	const struct proc *p;
	const struct user *up;
{
	static char envbuf[ARG_MAX + 1];
	register char *cp, *acp;
	int left, rv;
	struct ps_strings arginfo;

	if (up == NULL || p->p_pid == 0 || p->p_pid == 2)
		goto retemptyenv;

	if (kvm_procread(p, PS_STRINGS, (char *)&arginfo, sizeof(arginfo)) !=
		sizeof(arginfo))
		goto bad;

	cp = envbuf;
	acp = arginfo.ps_envstr;
	left = ARG_MAX + 1;
	while (arginfo.ps_nenvstr--) {
		if ((rv = kvm_procreadstr(p, acp, cp, left)) >= 0) {
			acp += rv + 1;
			left -= rv + 1;
			cp += rv;
			*cp++ = ' ';
			*cp = '\0';
		} else
			goto bad;
	}
	cp-- ; *cp = '\0';
	return (envbuf);

bad:
	seterr("error locating environment for pid %d", p->p_pid);
retemptyenv:
	envbuf[0] = '\0';
	return (envbuf);
}

static
getkvars()
{
	if (kvm_nlist(nl) == -1)
		return (-1);
	if (deadkernel) {
		/* We must do the sys map first because klseek uses it */
		long	addr;

#if defined(m68k)
		addr = (long) nl[X_LOWRAM].n_value;
		(void) lseek(kmem, addr, 0);
		if (read(kmem, (char *) &lowram, sizeof (lowram))
		    != sizeof (lowram)) {
			seterr("can't read lowram");
			return (-1);
		}
		lowram = btop(lowram);
		Sysseg = (struct ste *) malloc(NBPG);
		if (Sysseg == NULL) {
			seterr("out of space for Sysseg");
			return (-1);
		}
		addr = (long) nl[X_SYSSEG].n_value;
		(void) lseek(kmem, addr, 0);
		read(kmem, (char *)&addr, sizeof(addr));
		(void) lseek(kmem, (long)addr, 0);
		if (read(kmem, (char *) Sysseg, NBPG) != NBPG) {
			seterr("can't read Sysseg");
			return (-1);
		}
#endif
#if defined(i386)
		PTD = (struct pde *) malloc(NBPG);
		if (PTD == NULL) {
			seterr("out of space for PTD");
			return (-1);
		}
		addr = (long) nl[X_IdlePTD].n_value;
		(void) lseek(kmem, addr, 0);
		read(kmem, (char *)&addr, sizeof(addr));
		(void) lseek(kmem, (long)addr, 0);
		if (read(kmem, (char *) PTD, NBPG) != NBPG) {
			seterr("can't read PTD");
			return (-1);
		}
#endif
	}
	if (kvm_read((void *) nl[X_NSWAP].n_value, &nswap, sizeof (long)) == -1) {
		seterr("can't read nswap");
		return (-1);
	}
	if (kvm_read((void *) nl[X_DMMIN].n_value, &dmmin, sizeof (long)) == -1) {
		seterr("can't read dmmin");
		return (-1);
	}
	if (kvm_read((void *) nl[X_DMMAX].n_value, &dmmax, sizeof (long)) == -1) {
		seterr("can't read dmmax");
		return (-1);
	}
	if (kvm_read((void *) nl[X_VM_PAGE_HASH_MASK].n_value,
		     &vm_page_hash_mask, sizeof (long)) == -1) {
		seterr("can't read vm_page_hash_mask");
		return (-1);
	}
	if (kvm_read((void *) nl[X_VM_PAGE_BUCKETS].n_value,
		     &vm_page_buckets, sizeof (long)) == -1) {
		seterr("can't read vm_page_buckets");
		return (-1);
	}
	if (kvm_read((void *) nl[X_PAGE_SHIFT].n_value,
		     &page_shift, sizeof (long)) == -1) {
		seterr("can't read page_shift");
		return (-1);
	}

	return (0);
}

kvm_read(loc, buf, len)
	void *loc;
	void *buf;
{
	if (kvmfilesopen == 0 && kvm_openfiles(NULL, NULL, NULL) == -1)
		return (-1);
	klseek(kmem, (off_t) loc, 0);
	if (read(kmem, buf, len) != len) {
		seterr("error reading kmem at %x", loc);
		return (-1);
	}
	return (len);
}

static void
klseek(fd, loc, off)
	int fd;
	off_t loc;
	int off;
{

	if (deadkernel) {
		if ((loc = Vtophys(loc)) == -1)
			return;
	}
	(void) lseek(fd, (off_t)loc, off);
}

static off_t
Vtophys(loc)
	u_long	loc;
{
	off_t newloc = (off_t) -1;
#if defined(m68k)
	int p, ste, pte;

	ste = *(int *)&Sysseg[btos(loc)];
	if ((ste & SG_V) == 0) {
		seterr("vtophys: segment not valid");
		return((off_t) -1);
	}
	p = btop(loc & SG_PMASK);
	newloc = (ste & SG_FRAME) + (p * sizeof(struct pte));
	(void) lseek(mem, newloc, 0);
	if (read(mem, (char *)&pte, sizeof pte) != sizeof pte) {
		seterr("vtophys: cannot locate pte");
		return((off_t) -1);
	}
	newloc = pte & PG_FRAME;
	if (pte == PG_NV || newloc < (off_t)ptob(lowram)) {
		seterr("vtophys: page not valid");
		return((off_t) -1);
	}
	newloc = (newloc - (off_t)ptob(lowram)) + (loc & PGOFSET);
#endif
#ifdef i386
	struct pde pde;
	struct pte pte;
	int p;

	pde = PTD[loc >> PDSHIFT];
	if (pde.pd_v == 0) {
		seterr("vtophys: page directory entry not valid");
		return((off_t) -1);
	}
	p = btop(loc & PT_MASK);
	newloc = pde.pd_pfnum + (p * sizeof(struct pte));
	(void) lseek(kmem, (long)newloc, 0);
	if (read(kmem, (char *)&pte, sizeof pte) != sizeof pte) {
		seterr("vtophys: cannot obtain desired pte");
		return((off_t) -1);
	}
	newloc = pte.pg_pfnum;
	if (pte.pg_v == 0) {
		seterr("vtophys: page table entry not valid");
		return((off_t) -1);
	}
	newloc += (loc & PGOFSET);
#endif
	return((off_t) newloc);
}

/*
 * locate address of unwired or swapped page
 */

static int
vatosw(mp, vaddr, maddr, swb)
vm_map_t	mp;
vm_offset_t	vaddr;
vm_offset_t	*maddr;
struct swapblk	*swb;
{
	struct vm_object	vm_object;
	struct vm_map_entry	vm_entry;
	long			saddr, addr, off;
	int			i;

	saddr = addr = (long)mp->header.next;
#ifdef DEBUG
	fprintf(stderr, "vatosw: head=%x\n", &mp->header);
#endif
	for (i = 0; ; i++) {
		/* Weed through map entries until vaddr in range */
		if (kvm_read((void *) addr, &vm_entry, sizeof(vm_entry)) == -1) {
			setsyserr("vatosw: read vm_map_entry");
			return 0;
		}
#ifdef DEBUG
		fprintf(stderr, "vatosw: %d/%d, vaddr=%x, start=%x, end=%x ",
			i, mp->nentries, vaddr, vm_entry.start, vm_entry.end);
		fprintf(stderr, "addr=%x, next=%x\n", addr, vm_entry.next);
#endif
		if ((vaddr >= vm_entry.start) && (vaddr < vm_entry.end))
			if (vm_entry.object.vm_object != 0)
				break;
			else {
#ifdef DEBUG
				fprintf(stderr, "vatosw: no object\n");
#endif
				seterr("vatosw: no object\n");
				return 0;
			}

		addr = (long)vm_entry.next;

		if (addr == saddr) {
			seterr("vatosw: map not found\n");
			return 0;
		}
	}

	if (vm_entry.is_a_map || vm_entry.is_sub_map) {
#ifdef DEBUG
		fprintf(stderr, "vatosw: is a %smap\n",
			vm_entry.is_sub_map ? "sub " : "");
#endif
		seterr("vatosw: is a %smap\n",
		       vm_entry.is_sub_map ? "sub " : "");
		return 0;
	}

	/* Locate memory object */
	off = (vaddr - vm_entry.start) + vm_entry.offset;
	addr = (long)vm_entry.object.vm_object;
	while (1) {
		if (kvm_read((void *) addr, &vm_object, sizeof (vm_object)) == -1) {
			setsyserr("vatosw: read vm_object");
			return 0;
		}

#ifdef DEBUG
		fprintf(stderr, "vatosw: find page: object %#x offset %x\n",
			addr, off);
#endif

		/* Lookup in page queue */
		if ((i = findpage(addr, off, maddr)) != -1)
			return i;

		if (vm_object.pager != 0 &&
		    (i = pager_get(&vm_object, off, swb)) != -1)
			return i;

		if (vm_object.shadow == 0)
			break;

#ifdef DEBUG
		fprintf(stderr, "vatosw: shadow obj at %x: offset %x+%x\n",
			addr, off, vm_object.shadow_offset);
#endif

		addr = (long)vm_object.shadow;
		off += vm_object.shadow_offset;
	}

	seterr("vatosw: page not found\n");
	return 0;
}


int
pager_get(object, off, swb)
struct vm_object *object;
long off;
struct swapblk	*swb;
{
	struct pager_struct	pager;
	struct swpager		swpager;
	struct swblock		swblock;

	/* Find address in swap space */
	if (kvm_read(object->pager, &pager, sizeof (pager)) == -1) {
		setsyserr("pager_get: read pager");
		return 0;
	}
	if (pager.pg_type != PG_SWAP) {
		seterr("pager_get: weird pager\n");
		return 0;
	}

	/* Get swap pager data */
	if (kvm_read(pager.pg_data, &swpager, sizeof (swpager)) == -1) {
		setsyserr("pager_get: read swpager");
		return 0;
	}

	off += object->paging_offset;

	/* Read swap block array */
	if (kvm_read((void *) swpager.sw_blocks +
			(off/dbtob(swpager.sw_bsize)) * sizeof swblock,
			&swblock, sizeof (swblock)) == -1) {
		setsyserr("pager_get: read swblock");
		return 0;
	}

	off %= dbtob(swpager.sw_bsize);

	if (swblock.swb_mask & (1 << atop(off))) {
		swb->offset = dbtob(swblock.swb_block) + off;
		swb->size = dbtob(swpager.sw_bsize) - off;
		return 1;
	}

	return -1;
}

static int
findpage(object, offset, maddr)
long			object;
long			offset;
vm_offset_t		*maddr;
{
	queue_head_t	bucket;
	struct vm_page	mem;
	long		addr, baddr;

	baddr = vm_page_buckets +
		vm_page_hash(object,offset) * sizeof(queue_head_t);

	if (kvm_read((void *) baddr, &bucket, sizeof (bucket)) == -1) {
		seterr("can't read vm_page_bucket");
		return 0;
	}

	addr = (long)bucket.next;

	while (addr != baddr) {
		if (kvm_read((void *) addr, &mem, sizeof (mem)) == -1) {
			seterr("can't read vm_page");
			return 0;
		}

		if ((long)mem.object == object && mem.offset == offset) {
			*maddr = (long)mem.phys_addr;
			return 1;
		}

		addr = (long)mem.hashq.next;
	}

	return -1;
}

#include <varargs.h>
static char errbuf[_POSIX2_LINE_MAX];

static void
seterr(va_alist)
	va_dcl
{
	char *fmt;
	va_list ap;

	va_start(ap);
	fmt = va_arg(ap, char *);
	(void) vsnprintf(errbuf, _POSIX2_LINE_MAX, fmt, ap);
#ifdef DEBUG
	(void) printf("%s", errbuf);
#endif
	va_end(ap);
}

static void
setsyserr(va_alist)
	va_dcl
{
	char *fmt, *cp;
	va_list ap;
	extern int errno;

	va_start(ap);
	fmt = va_arg(ap, char *);
	(void) vsnprintf(cp = errbuf, _POSIX2_LINE_MAX, fmt, ap);
	cp += strlen(cp);
	(void) snprintf(cp, _POSIX2_LINE_MAX - (cp - errbuf), ": %s",
			strerror(errno));
#ifdef DEBUG
	(void) printf("%s", errbuf);
#endif
	va_end(ap);
}

char *
kvm_geterr()
{
	return (errbuf);
}
