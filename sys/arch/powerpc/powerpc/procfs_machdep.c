/*	$NetBSD: procfs_machdep.c,v 1.1 2003/03/05 05:27:24 matt Exp $	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <miscfs/procfs/procfs.h>

#include <machine/reg.h>


/*
 * Linux-style /proc/cpuinfo.
 * Only used when procfs is mounted with -o linux.
 */
int
procfs_getcpuinfstr(char *buf, int *len)
{
	*len = 0;

	return 0;
}

#ifdef __HAVE_PROCFS_MACHDEP
void
procfs_machdep_allocvp(struct vnode *vp)
{
	struct pfsnode *pfs = vp->v_data;

	switch (pfs->pfs_type) {
	case Pmachdep_vecregs:	/* /proc/N/vecregs = -rw------- */
		pfs->pfs_mode = S_IRUSR|S_IWUSR;
		vp->v_type = VREG;
		break;

	default:
		panic("procfs_machdep_allocvp");
	}
}

int
procfs_machdep_rw(struct proc *curp, struct lwp *l, struct pfsnode *pfs,
    struct uio *uio)
{

	switch (pfs->pfs_type) {
	case Pmachdep_vecregs:
		return (procfs_machdep_dovecregs(curp, l, pfs, uio));

	default:
		panic("procfs_machdep_rw");
	}

	/* NOTREACHED */
	return (EINVAL);
}

int
procfs_machdep_getattr(struct vnode *vp, struct vattr *vap, struct proc *procp)
{
	struct pfsnode *pfs = VTOPFS(vp);

	switch (pfs->pfs_type) {
	case Pmachdep_vecregs:
		vap->va_bytes = vap->va_size = sizeof(struct vreg);
		break;

	default:
		panic("procfs_machdep_getattr");
	}

	return (0);
}

int
procfs_machdep_dovecregs(struct proc *curp, struct lwp *l,
    struct pfsnode *pfs, struct uio *uio)
{

	return (process_machdep_dovecregs(curp, l, uio));
}

int
procfs_machdep_validvecregs(struct proc *p, struct mount *mp)
{

	return (process_machdep_validvecregs(p));
}
#endif
