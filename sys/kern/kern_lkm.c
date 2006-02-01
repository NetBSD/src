/*	$NetBSD: kern_lkm.c,v 1.88.2.1 2006/02/01 14:52:20 yamt Exp $	*/

/*
 * Copyright (c) 1994 Christopher G. Demetriou
 * Copyright (c) 1992 Terrence R. Lambert.
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
 *      This product includes software developed by Terrence R. Lambert.
 * 4. The name Terrence R. Lambert may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TERRENCE R. LAMBERT ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE TERRENCE R. LAMBERT BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * XXX it's not really safe to unload *any* of the types which are
 * currently loadable; e.g. you could unload a syscall which was being
 * blocked in, etc.  In the long term, a solution should be come up
 * with, but "not right now." -- cgd
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_lkm.c,v 1.88.2.1 2006/02/01 14:52:20 yamt Exp $");

#include "opt_ddb.h"
#include "opt_malloclog.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/conf.h>
#include <sys/ksyms.h>
#include <sys/device.h>
#include <sys/once.h>

#include <sys/lkm.h>
#include <sys/syscall.h>
#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#endif

#include <uvm/uvm_extern.h>

struct vm_map *lkm_map;

#define	LKM_SPACE_ALLOC(size) \
	uvm_km_alloc(lkm_map, (size), 0, UVM_KMF_WIRED)
#define	LKM_SPACE_FREE(addr, size) \
	uvm_km_free(lkm_map, (addr), (size), UVM_KMF_WIRED)

#if !defined(DEBUG) && defined(LKMDEBUG)
# define DEBUG
#endif

#ifdef DEBUG
# define LKMDB_INFO	0x01
# define LKMDB_LOAD	0x02
int	lkmdebug = 0;
#endif

#define	LKM_ALLOC	0x01

#define	LKMS_IDLE	0x00
#define	LKMS_RESERVED	0x01
#define	LKMS_LOADING	0x02
#define	LKMS_UNLOADING	0x08

static int	lkm_v = 0;
static int	lkm_state = LKMS_IDLE;

static TAILQ_HEAD(lkms_head, lkm_table) lkmods = /* table of loaded modules */
		TAILQ_HEAD_INITIALIZER(lkmods);
static struct lkm_table	*curp;			/* global for in-progress ops */

static struct lkm_table *lkmlookup(int, char *, int, int *);
static struct lkm_table *lkmalloc(void);
static void lkmfree(void);
static void lkmunreserve(int);
static int _lkm_syscall(struct lkm_table *, int);
static int _lkm_vfs(struct lkm_table *, int);
static int _lkm_dev(struct lkm_table *, int);
#ifdef STREAMS
static int _lkm_strmod(struct lkm_table *, int);
#endif
static int _lkm_exec(struct lkm_table *, int);
static int _lkm_compat(struct lkm_table *, int);
static int _lkm_drv(struct lkm_table *, int);

static int _lkm_checkver(struct lkm_table *);

dev_type_open(lkmopen);
dev_type_close(lkmclose);
dev_type_ioctl(lkmioctl);

const struct cdevsw lkm_cdevsw = {
	lkmopen, lkmclose, noread, nowrite, lkmioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

static ONCE_DECL(lkm_init_once);

static int
lkm_init(void)
{
	/*
	 * If machine-dependent code hasn't initialized the lkm_map
	 * then just use kernel_map.
	 */
	if (lkm_map == NULL)
		lkm_map = kernel_map;

	return 0;
}

/*ARGSUSED*/
int
lkmopen(dev_t dev, int flag, int devtype, struct lwp *l)
{
	int error;

	RUN_ONCE(&lkm_init_once, lkm_init);

	if (minor(dev) != 0)
		return (ENXIO);		/* bad minor # */

	/*
	 * Use of the loadable kernel module device must be exclusive; we
	 * may try to remove this restriction later, but it's really no
	 * hardship.
	 */
	while (lkm_v & LKM_ALLOC) {
		if (flag & FNONBLOCK)		/* don't hang */
			return (EBUSY);
		/*
		 * Sleep pending unlock; we use tsleep() to allow
		 * an alarm out of the open.
		 */
		error = tsleep((caddr_t)&lkm_v, TTIPRI|PCATCH, "lkmopn", 0);
		if (error)
			return (error);
	}
	lkm_v |= LKM_ALLOC;

	return (0);		/* pseudo-device open */
}

/*
 * Look up for a LKM in the list.
 */
static struct lkm_table *
lkmlookup(int i, char *name, int need_copyin, int *error)
{
	struct lkm_table *p;
	char istr[MAXLKMNAME];

	/*
	 * p being NULL here implies the list is empty, so any lookup is
	 * invalid (name based or otherwise). Since the list of modules is
	 * kept sorted by id, lowest to highest, the id of the last entry
	 * will be the highest in use.
	 */
	p = TAILQ_LAST(&lkmods, lkms_head);
	if (p == NULL || i > p->id) {
		*error = EINVAL;
		return (NULL);
	}

	if (i < 0) {		/* unload by name */
		/*
		 * Copy name and lookup id from all loaded
		 * modules.  May fail.
		 */
		if (need_copyin) {
			*error = copyinstr(name, istr, MAXLKMNAME - 1, NULL);
			if (*error)
				return (NULL);
		} else
			strncpy(istr, name, MAXLKMNAME - 1);
		istr[MAXLKMNAME - 1] = '\0';

		TAILQ_FOREACH(p, &lkmods, link) {
			if (strcmp(istr, p->private.lkm_any->lkm_name) == 0)
				break;
		}
	} else
		TAILQ_FOREACH(p, &lkmods, link)
			if (i == p->id)
				break;

	if (p == NULL)
		*error = ENOENT;

	return (p);
}

/*
 * Allocates memory for a new LKM table entry and inserts in the list.
 * Returns NULL on failure.
 */
static struct lkm_table *
lkmalloc(void)
{
	struct lkm_table *p, *ret;
	int id = 0;

	ret = malloc(sizeof(struct lkm_table), M_DEVBUF, M_NOWAIT);
	if (ret == NULL)
		return (NULL);
	ret->refcnt = 0;
	ret->forced = 0;

	/* find the first unused id */
	TAILQ_FOREACH(p, &lkmods, link) {
		if (id != p->id)
			break;
		id++;
	}
	ret->id = id;

	if (p == NULL)
		TAILQ_INSERT_TAIL(&lkmods, ret, link);
	else
		TAILQ_INSERT_BEFORE(p, ret, link);

	return (ret);
}

/*
 * Frees the current LKM table entry.
 */
static void
lkmfree(void)
{
	TAILQ_REMOVE(&lkmods, curp, link);
	free(curp, M_DEVBUF);
	curp = NULL;
}

/*
 * Unreserve the memory associated with the current loaded module; done on
 * a coerced close of the lkm device (close on premature exit of modload)
 * or explicitly by modload as a result of a link failure.
 */
static void
lkmunreserve(int delsymtab)
{

	if (lkm_state == LKMS_IDLE)
		return;

	if (curp && curp->syms) {
		if (delsymtab)
			ksyms_delsymtab(curp->private.lkm_any->lkm_name);
		LKM_SPACE_FREE(curp->syms, curp->sym_size);
		curp->syms = 0;
	}
	/*
	 * Actually unreserve the memory
	 */
	if (curp && curp->area) {
		LKM_SPACE_FREE(curp->area, curp->size);
		curp->area = 0;
	}

	if (curp && curp->forced)
		curp->forced = 0;

	lkm_state = LKMS_IDLE;
}

int
lkmclose(dev_t dev, int flag, int mode, struct lwp *l)
{

	if (!(lkm_v & LKM_ALLOC)) {
#ifdef DEBUG
		if (lkmdebug & LKMDB_INFO)
			printf("LKM: close before open!\n");
#endif	/* DEBUG */
		return (EBADF);
	}

	/* do this before waking the herd... */
	if (curp != NULL && curp->refcnt == 0) {
		/*
		 * If we close before setting used, we have aborted
		 * by way of error or by way of close-on-exit from
		 * a premature exit of "modload".
		 */
		lkmunreserve(1);	/* coerce state to LKM_IDLE */
		lkmfree();
	}

	lkm_v &= ~LKM_ALLOC;
	wakeup((caddr_t)&lkm_v);	/* thundering herd "problem" here */

	return (0);		/* pseudo-device closed */
}

/*ARGSUSED*/
int
lkmioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	int i, error = 0;
	struct lmc_resrv *resrvp;
	struct lmc_loadbuf *loadbufp;
	struct lmc_unload *unloadp;
	struct lmc_stat	 *statp;

	switch(cmd) {
	case LMRESERV:		/* reserve pages for a module */
		if (securelevel > 0)
			return EPERM;

		if ((flag & FWRITE) == 0) /* only allow this if writing */
			return EPERM;

		resrvp = (struct lmc_resrv *)data;

		curp = lkmalloc();
		if (curp == NULL) {
			error = ENOMEM;
			break;
		}
		resrvp->slot = curp->id;	/* return slot */

		/*
		 * Get memory for module
		 */
		curp->size = resrvp->size;
		curp->area = LKM_SPACE_ALLOC(curp->size);
		curp->offset = 0;		/* load offset */

		resrvp->addr = curp->area;	/* ret kernel addr */

		if (resrvp->sym_size) {
			curp->sym_size = resrvp->sym_size;
			curp->sym_symsize = resrvp->sym_symsize;
			curp->syms = (u_long) LKM_SPACE_ALLOC(curp->sym_size);
			curp->sym_offset = 0;
			resrvp->sym_addr = curp->syms; /* ret symbol addr */
		} else {
			curp->sym_size = 0;
			curp->syms = 0;
			curp->sym_offset = 0;
			resrvp->sym_addr = 0;
		}

#ifdef DEBUG
		if (lkmdebug & LKMDB_INFO) {
			printf("LKM: LMRESERV (actual   = 0x%08lx)\n",
			    curp->area);
			printf("LKM: LMRESERV (syms     = 0x%08lx)\n",
			       curp->syms);
			printf("LKM: LMRESERV (adjusted = 0x%08lx)\n",
			    trunc_page(curp->area));
		}
#endif /* DEBUG */
		lkm_state = LKMS_RESERVED;
		break;

	case LMLOADBUF:		/* Copy in; stateful, follows LMRESERV */
		if (securelevel > 0)
			return EPERM;

		if ((flag & FWRITE) == 0) /* only allow this if writing */
			return EPERM;

		loadbufp = (struct lmc_loadbuf *)data;
		i = loadbufp->cnt;
		if ((lkm_state != LKMS_RESERVED && lkm_state != LKMS_LOADING)
		    || i < 0
		    || i > MODIOBUF
		    || i > curp->size - curp->offset) {
			error = ENOMEM;
			break;
		}

		/* copy in buffer full of data */
		error = copyin(loadbufp->data,
			       (caddr_t)curp->area + curp->offset, i);
		if (error)
			break;

#ifdef PMAP_NEED_PROCWR
		pmap_procwr(&proc0, curp->area + curp->offset, i);
#endif
		if ((curp->offset + i) < curp->size) {
			lkm_state = LKMS_LOADING;
#ifdef DEBUG
			if (lkmdebug & LKMDB_LOAD)
		printf("LKM: LMLOADBUF (loading @ %ld of %ld, i = %d)\n",
			    curp->offset, curp->size, i);
#endif /* DEBUG */
		}
		curp->offset += i;
		break;

	case LMLOADSYMS:	/* Copy in; stateful, follows LMRESERV*/
		if ((flag & FWRITE) == 0) /* only allow this if writing */
			return EPERM;

		loadbufp = (struct lmc_loadbuf *)data;
		i = loadbufp->cnt;
		if ((lkm_state != LKMS_LOADING)
		    || i < 0
		    || i > MODIOBUF
		    || i > curp->sym_size - curp->sym_offset) {
			error = ENOMEM;
			break;
		}

		/* copy in buffer full of data*/
		if ((error = copyin(loadbufp->data,
				   (caddr_t)(curp->syms) + curp->sym_offset,
				   i)) != 0)
			break;

		if ((curp->sym_offset + i) < curp->sym_size) {
			lkm_state = LKMS_LOADING;
#ifdef DEBUG
			if (lkmdebug & LKMDB_LOAD)
		printf( "LKM: LMLOADSYMS (loading @ %ld of %ld, i = %d)\n",
			curp->sym_offset, curp->sym_size, i);
#endif	/* DEBUG*/
		}
		curp->sym_offset += i;
		break;

	case LMUNRESRV:		/* discard reserved pages for a module */
		if (securelevel > 0)
			return EPERM;

		if ((flag & FWRITE) == 0) /* only allow this if writing */
			return EPERM;

		lkmunreserve(0);	/* coerce state to LKM_IDLE */
		if (curp != NULL)
			lkmfree();
#ifdef DEBUG
		if (lkmdebug & LKMDB_INFO)
			printf("LKM: LMUNRESERV\n");
#endif /* DEBUG */
		break;

	case LMREADY:		/* module loaded: call entry */
		if (securelevel > 0)
			return EPERM;

		if ((flag & FWRITE) == 0) /* only allow this if writing */
			return EPERM;

		if (lkm_state != LKMS_LOADING) {
#ifdef DEBUG
			if (lkmdebug & LKMDB_INFO)
				printf("lkm_state is %02x\n", lkm_state);
#endif /* DEBUG */
			return ENXIO;
		}

		if (curp->size - curp->offset > 0) {
			/* The remainder must be bss, so we clear it */
			memset((caddr_t)curp->area + curp->offset, 0,
			       curp->size - curp->offset);
		}

#ifdef DDB
		/*
		 * Temporarily load the symbol table before the entry
		 * routine is called, so that the symbols are available
		 * for DDB backtrace and breakpoints.
		 */
		if (curp->syms && curp->sym_offset >= curp->sym_size) {
			error = ksyms_addsymtab("/lkmtemp/",
			    (char *)curp->syms, curp->sym_symsize,
			    (char *)curp->syms + curp->sym_symsize,
			    curp->sym_size - curp->sym_symsize);

			if (error)
				goto rdyfail;

#ifdef DEBUG
			if (lkmdebug & LKMDB_INFO)
				printf( "DDB symbols added!\n" );
#endif
		}
#endif /* DDB */

		curp->entry = (int (*)(struct lkm_table *, int, int))
				(*((long *) (data)));

		/* call entry(load)... (assigns "private" portion) */
		error = (*(curp->entry))(curp, LKM_E_LOAD, LKM_VERSION);

		if (curp->syms && curp->sym_offset >= curp->sym_size) {
#ifdef DDB
			ksyms_delsymtab("/lkmtemp/");
#endif

			if (!error) {
				error = ksyms_addsymtab(curp->private.lkm_any->lkm_name,
				    (char *)curp->syms, curp->sym_symsize,
				    (char *)curp->syms + curp->sym_symsize,
				    curp->sym_size - curp->sym_symsize);
			}
		}

		if (error) {
#ifdef DDB
    rdyfail:
#endif
			/*
			 * Module may refuse loading or may have a
			 * version mismatch...
			 */
			lkm_state = LKMS_UNLOADING;	/* for lkmunreserve */
			lkmunreserve(0);		/* free memory */
			lkmfree();			/* free slot */
#ifdef DEBUG
			if (lkmdebug & LKMDB_INFO)
				printf("lkm entry point failed with error %d\n",
				   error);
#endif /* DEBUG */
			break;
		}
		curp->refcnt++;

#ifdef DEBUG
		if (lkmdebug & LKMDB_INFO)
			printf("LKM: LMREADY\n");
#endif /* DEBUG */
		lkm_state = LKMS_IDLE;
		break;

	case LMUNLOAD:		/* unload a module */
		if (securelevel > 0)
			return EPERM;

		if ((flag & FWRITE) == 0) /* only allow this if writing */
			return EPERM;

		unloadp = (struct lmc_unload *)data;

		curp = lkmlookup(unloadp->id, unloadp->name, 1, &error);
		if (curp == NULL)
			break;

		/* call entry(unload) */
		if ((*(curp->entry))(curp, LKM_E_UNLOAD, LKM_VERSION)) {
			error = EBUSY;
			break;
		}

		lkm_state = LKMS_UNLOADING;	/* non-idle for lkmunreserve */
		lkmunreserve(1);		/* free memory */
		lkmfree();			/* free slot */
		break;

	case LMSTAT:		/* stat a module by id/name */
		/* allow readers and writers to stat */

		statp = (struct lmc_stat *)data;

		if ((curp = lkmlookup(statp->id, statp->name, 0, &error)) == NULL)
			break;

		if ((error = (*curp->entry)(curp, LKM_E_STAT, LKM_VERSION)))
			break;

		/*
		 * Copy out stat information for this module...
		 */
		statp->id	= curp->id;
		statp->offset	= curp->private.lkm_any->lkm_offset;
		statp->type	= curp->private.lkm_any->lkm_type;
		statp->area	= curp->area;
		statp->size	= curp->size / 1024;
		statp->private	= (unsigned long)curp->private.lkm_any;
		statp->ver	= LKM_VERSION;
		copystr(curp->private.lkm_any->lkm_name,
			  statp->name,
			  MAXLKMNAME - 2,
			  (size_t *)0);

		break;

#ifdef LMFORCE
	case LMFORCE:		/* stateful, optionally follows LMRESERV */
		if (securelevel > 0)
			return EPERM;

		if ((flag & FWRITE) == 0) /* only allow this if writing */
			return EPERM;

		if (lkm_state != LKMS_RESERVED) {
			error = EPERM;
			break;
		}

		curp->forced = (*(u_long *)data != 0);
		break;
#endif /* LMFORCE */

	default:		/* bad ioctl()... */
		error = ENOTTY;
		break;
	}

	return (error);
}

/*
 * Acts like "nosys" but can be identified in sysent for dynamic call
 * number assignment for a limited number of calls.
 *
 * Place holder for system call slots reserved for loadable modules.
 */
int
sys_lkmnosys(struct lwp *l, void *v, register_t *retval)
{

	return (sys_nosys(l, v, retval));
}

/*
 * A placeholder function for load/unload/stat calls; simply returns zero.
 * Used where people don't want to specify a special function.
 */
int
lkm_nofunc(struct lkm_table *lkmtp, int cmd)
{

	return (0);
}

int
lkmexists(struct lkm_table *lkmtp)
{
	struct lkm_table *p;

	/* see if name exists... */
	TAILQ_FOREACH(p, &lkmods, link) {
		if (strcmp(lkmtp->private.lkm_any->lkm_name,
		    p->private.lkm_any->lkm_name) == 0 && (p->refcnt != 0))
			return (1);		/* already loaded... */
	}

	return (0);		/* module not loaded... */
}

/*
 * For the loadable system call described by the structure pointed to
 * by lkmtp, load/unload/stat it depending on the cmd requested.
 */
static int
_lkm_syscall(struct lkm_table *lkmtp, int cmd)
{
	struct lkm_syscall *args = lkmtp->private.lkm_syscall;
	int i;
	int error = 0;

	switch(cmd) {
	case LKM_E_LOAD:
		/* don't load twice! */
		if (lkmexists(lkmtp))
			return (EEXIST);

		if ((i = args->mod.lkm_offset) == -1) {	/* auto */
			/*
			 * Search the table looking for a slot...
			 */
			for (i = 0; i < SYS_MAXSYSCALL; i++)
				if (sysent[i].sy_call == sys_lkmnosys)
					break;		/* found it! */
			/* out of allocable slots? */
			if (i == SYS_MAXSYSCALL) {
				error = ENFILE;
				break;
			}
		} else {				/* assign */
			if (i < 0 || i >= SYS_MAXSYSCALL) {
				error = EINVAL;
				break;
			}
		}

		/* save old */
		memcpy(&args->lkm_oldent, &sysent[i], sizeof(struct sysent));

		/* replace with new */
		memcpy(&sysent[i], args->lkm_sysent, sizeof(struct sysent));

		/* done! */
		args->mod.lkm_offset = i;	/* slot in sysent[] */

		break;

	case LKM_E_UNLOAD:
		/* current slot... */
		i = args->mod.lkm_offset;

		/* replace current slot contents with old contents */
		memcpy(&sysent[i], &args->lkm_oldent, sizeof(struct sysent));

		break;

	case LKM_E_STAT:	/* no special handling... */
		break;
	}

	return (error);
}

/*
 * For the loadable virtual file system described by the structure pointed
 * to by lkmtp, load/unload/stat it depending on the cmd requested.
 */
static int
_lkm_vfs(struct lkm_table *lkmtp, int cmd)
{
	struct lkm_vfs *args = lkmtp->private.lkm_vfs;
	int error = 0;

	switch(cmd) {
	case LKM_E_LOAD:
		/* don't load twice! */
		if (lkmexists(lkmtp))
			return (EEXIST);

		/* Establish the file system. */
		if ((error = vfs_attach(args->lkm_vfsops)) != 0)
			return (error);

		/* done! */
		break;

	case LKM_E_UNLOAD:
		/* Disestablish the file system. */
		if ((error = vfs_detach(args->lkm_vfsops)) != 0)
			return (error);
		break;

	case LKM_E_STAT:	/* no special handling... */
		break;
	}

	return (error);
}

/*
 * For the loadable device driver described by the structure pointed to
 * by lkmtp, load/unload/stat it depending on the cmd requested.
 */
static int
_lkm_dev(struct lkm_table *lkmtp, int cmd)
{
	struct lkm_dev *args = lkmtp->private.lkm_dev;
	int error;

	switch(cmd) {
	case LKM_E_LOAD:
		/* don't load twice! */
		if (lkmexists(lkmtp))
			return (EEXIST);

		error = devsw_attach(args->lkm_devname,
				     args->lkm_bdev, &args->lkm_bdevmaj,
				     args->lkm_cdev, &args->lkm_cdevmaj);
		if (error != 0)
			return (error);

		args->mod.lkm_offset =
			LKM_MAKEMAJOR(args->lkm_bdevmaj, args->lkm_cdevmaj);
		break;

	case LKM_E_UNLOAD:
		devsw_detach(args->lkm_bdev, args->lkm_cdev);
		args->lkm_bdevmaj = -1;
		args->lkm_cdevmaj = -1;
		break;

	case LKM_E_STAT:	/* no special handling... */
		break;
	}

	return (0);
}

#ifdef STREAMS
/*
 * For the loadable streams module described by the structure pointed to
 * by lkmtp, load/unload/stat it depending on the cmd requested.
 */
static int
_lkm_strmod(struct lkm_table *lkmtp, int cmd)
{
	struct lkm_strmod *args = lkmtp->private.lkm_strmod;
	int i;
	int error = 0;

	switch(cmd) {
	case LKM_E_LOAD:
		/* don't load twice! */
		if (lkmexists(lkmtp))
			return (EEXIST);
		break;

	case LKM_E_UNLOAD:
		break;

	case LKM_E_STAT:	/* no special handling... */
		break;
	}

	return (error);
}
#endif	/* STREAMS */

/*
 * For the loadable execution class described by the structure pointed to
 * by lkmtp, load/unload/stat it depending on the cmd requested.
 */
static int
_lkm_exec(struct lkm_table *lkmtp, int cmd)
{
	struct lkm_exec *args = lkmtp->private.lkm_exec;
	int error = 0;

	switch(cmd) {
	case LKM_E_LOAD:
		/* don't load twice! */
		if (lkmexists(lkmtp))
			return (EEXIST);

		/* this would also fill in the emulation pointer in
		 * args->lkm_execsw */
		error = exec_add(args->lkm_execsw, args->lkm_emul);
		break;

	case LKM_E_UNLOAD:
		error = exec_remove(args->lkm_execsw);
		break;

	case LKM_E_STAT:	/* no special handling... */
		break;
	}

	return (error);
}

/*
 * For the loadable compat/emulation class described by the structure pointed to
 * by lkmtp, load/unload/stat it depending on the cmd requested.
 */
static int
_lkm_compat(struct lkm_table *lkmtp, int cmd)
{
	struct lkm_compat *args = lkmtp->private.lkm_compat;
	int error = 0;

	switch(cmd) {
	case LKM_E_LOAD:
		/* don't load twice! */
		if (lkmexists(lkmtp))
			return (EEXIST);

		error = emul_register(args->lkm_compat, 0);
		break;

	case LKM_E_UNLOAD:
		error = emul_unregister(args->lkm_compat->e_name);
		break;

	case LKM_E_STAT:	/* no special handling... */
		break;
	}

	return (error);
}

static int
drvlkm_load(struct cfdriver **cd, const struct cfattachlkminit *cai,
	    struct cfdata *cf)
{
	const struct cfattachlkminit *cfai;
	int i, error, j;

	for (i = 0; cd[i]; i++) {
		error = config_cfdriver_attach(cd[i]);
		if (!error)
			continue;
		if (error != EEXIST) {
			printf("%s: unable to register driver\n",
			       cd[i]->cd_name);
			/* XXX roll back previous attachments */
			goto out;
		}
		printf("driver %s already present\n", cd[i]->cd_name);
		/*
		 * get existing drivers out of the list so we won't try
		 * to detach them
		 */
		for (j = i; cd[j]; j++)
			cd[j] = cd[j + 1];
		i--; /* continue at same index */
	}

	for (cfai = cai; cfai->cfai_name; cfai++) {
		for (i = 0; cfai->cfai_list[i]; i++) {
			error = config_cfattach_attach(cfai->cfai_name,
						       cfai->cfai_list[i]);
			if (!error)
				continue;
			if (error != EEXIST) {
				printf("%s: unable to register cfattach\n",
				       cfai->cfai_list[i]->ca_name);
				/* XXX roll back previous attachments */
				goto out;
			}
			printf("driver attachment %s for %s already present\n",
			       cfai->cfai_list[i]->ca_name, cfai->cfai_name);
			/*
			 * get existing attachments out of the list so we
			 * won't try to detach them
			 */
			for (j = i; cfai->cfai_list[j]; j++)
				cfai->cfai_list[j] = cfai->cfai_list[j + 1];
			i--; /* continue at same index */
		}
	}

	error = config_cfdata_attach(cf, 1);
	/* XXX roll back cfdriver / cfattach attachments in error case */

out:
	return (error);
}

static int
drvlkm_unload(struct cfdriver **cd, const struct cfattachlkminit *cai,
	      struct cfdata *cf)
{
	const struct cfattachlkminit *cfai;
	int i, error;

	error = config_cfdata_detach(cf);
	if (error)
		return (error);

	for (cfai = cai; cfai->cfai_name; cfai++) {
		for (i = 0; cfai->cfai_list[i]; i++) {
			error = config_cfattach_detach(cfai->cfai_name,
						       cfai->cfai_list[i]);
			if (error) {
				printf("%s: unable to deregister cfattach\n",
				       cfai->cfai_list[i]->ca_name);
				return (error);
			}
		}
	}

	for (i = 0; cd[i]; i++) {
		error = config_cfdriver_detach(cd[i]);
		if (error) {
			printf("%s: unable to deregister cfdriver\n",
	    			cd[i]->cd_name);
			return (error);
		}
	}

	return (0);
}

static int
_lkm_drv(struct lkm_table *lkmtp, int cmd)
{
	struct lkm_drv *args = lkmtp->private.lkm_drv;
	int error = 0;

	switch(cmd) {
	case LKM_E_LOAD:
		/* don't load twice! */
		if (lkmexists(lkmtp))
			return (EEXIST);

		error = drvlkm_load(args->lkm_cd,
				    args->lkm_cai,
				    args->lkm_cf);
		break;

	case LKM_E_UNLOAD:
		error = drvlkm_unload(args->lkm_cd,
				      args->lkm_cai,
				      args->lkm_cf);
		break;

	case LKM_E_STAT:	/* no special handling... */
		break;
	}

	return (error);
}

/*
 * This code handles the per-module type "wiring-in" of loadable modules
 * into existing kernel tables.  For "LM_MISC" modules, wiring and unwiring
 * is assumed to be done in their entry routines internal to the module
 * itself.
 */
int
lkmdispatch(struct lkm_table *lkmtp, int cmd)
{
	int error = 0;		/* default = success */
#ifdef DEBUG
	if (lkmdebug & LKMDB_INFO)
		printf( "lkmdispatch: %p %d\n", lkmtp, cmd );
#endif

	/* If loading, check the LKM is compatible */
	if (cmd == LKM_E_LOAD) {
		if (_lkm_checkver(lkmtp))
			return (EPROGMISMATCH);
	}

	switch(lkmtp->private.lkm_any->lkm_type) {
	case LM_SYSCALL:
		error = _lkm_syscall(lkmtp, cmd);
		break;

	case LM_VFS:
		error = _lkm_vfs(lkmtp, cmd);
		break;

	case LM_DEV:
		error = _lkm_dev(lkmtp, cmd);
		break;

#ifdef STREAMS
	case LM_STRMOD:
	    {
		struct lkm_strmod *args = lkmtp->private.lkm_strmod;
	    }
		break;

#endif	/* STREAMS */

	case LM_EXEC:
		error = _lkm_exec(lkmtp, cmd);
		break;

	case LM_COMPAT:
		error = _lkm_compat(lkmtp, cmd);
		break;

	case LM_MISC:	/* ignore content -- no "misc-specific" procedure */
		break;

	case LM_DRV:
		error = _lkm_drv(lkmtp, cmd);
		break;

	default:
		error = ENXIO;	/* unknown type */
		break;
	}

	return (error);
}

/*
 * Check LKM version against current kernel.
 */
static int
_lkm_checkver(struct lkm_table *lkmtp)
{
	struct lkm_any *mod = lkmtp->private.lkm_any;

	if (mod->lkm_modver != LKM_VERSION) {
		printf("LKM '%s': LKM version mismatch - LKM %d, kernel %d\n",
		    mod->lkm_name, mod->lkm_modver, LKM_VERSION);
		return (1);
	}

	if (lkmtp->forced) {
		printf("LKM '%s': forced load, skipping compatibility checks\n",
		    mod->lkm_name);
		return (0);
	}

	if (mod->lkm_sysver != __NetBSD_Version__) {
		printf("LKM '%s': kernel version mismatch - LKM %d, kernel %d\n",
		    mod->lkm_name, mod->lkm_sysver, __NetBSD_Version__);
		return (2);
	}

	/*
	 * Following might eventually be changed to take into account envdep,
	 * if it's non-NULL.
	 */
	if (strcmp(mod->lkm_envver, _LKM_ENV_VERSION) != 0) {
		const char *kenv = _LKM_ENV_VERSION;
		const char *envver = mod->lkm_envver;

		if (kenv[0] == ',')
			kenv++;
		if (envver[0] == ',')
			envver++;

		printf("LKM '%s': environment compile options mismatch - LKM '%s', kernel '%s'\n",
		    mod->lkm_name, envver, kenv);
		return (3);
	}

	/*
	 * Basic parameters match, LKM is hopefully compatible.
	 * Cross fingers and approve.
	 */
	return (0);
}
