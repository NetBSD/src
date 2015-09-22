/*	$NetBSD: coda_psdev.c,v 1.53.4.3 2015/09/22 12:05:55 skrll Exp $	*/

/*
 *
 *             Coda: an Experimental Distributed File System
 *                              Release 3.1
 *
 *           Copyright (c) 1987-1998 Carnegie Mellon University
 *                          All Rights Reserved
 *
 * Permission  to  use, copy, modify and distribute this software and its
 * documentation is hereby granted,  provided  that  both  the  copyright
 * notice  and  this  permission  notice  appear  in  all  copies  of the
 * software, derivative works or  modified  versions,  and  any  portions
 * thereof, and that both notices appear in supporting documentation, and
 * that credit is given to Carnegie Mellon University  in  all  documents
 * and publicity pertaining to direct or indirect use of this code or its
 * derivatives.
 *
 * CODA IS AN EXPERIMENTAL SOFTWARE SYSTEM AND IS  KNOWN  TO  HAVE  BUGS,
 * SOME  OF  WHICH MAY HAVE SERIOUS CONSEQUENCES.  CARNEGIE MELLON ALLOWS
 * FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION.   CARNEGIE  MELLON
 * DISCLAIMS  ANY  LIABILITY  OF  ANY  KIND  FOR  ANY  DAMAGES WHATSOEVER
 * RESULTING DIRECTLY OR INDIRECTLY FROM THE USE OF THIS SOFTWARE  OR  OF
 * ANY DERIVATIVE WORK.
 *
 * Carnegie  Mellon  encourages  users  of  this  software  to return any
 * improvements or extensions that  they  make,  and  to  grant  Carnegie
 * Mellon the rights to redistribute these changes without encumbrance.
 *
 * 	@(#) coda/coda_psdev.c,v 1.1.1.1 1998/08/29 21:26:45 rvb Exp $
 */

/*
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

/*
 * This code was written for the Coda file system at Carnegie Mellon
 * University.  Contributers include David Steere, James Kistler, and
 * M. Satyanarayanan.  */

/* These routines define the pseudo device for communication between
 * Coda's Venus and Minicache in Mach 2.6. They used to be in cfs_subr.c,
 * but I moved them to make it easier to port the Minicache without
 * porting coda. -- DCS 10/12/94
 *
 * Following code depends on file-system CODA.
 */

/* These routines are the device entry points for Venus. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: coda_psdev.c,v 1.53.4.3 2015/09/22 12:05:55 skrll Exp $");

extern int coda_nc_initialized;    /* Set if cache has been initialized */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/conf.h>
#include <sys/atomic.h>
#include <sys/module.h>

#include <coda/coda.h>
#include <coda/cnode.h>
#include <coda/coda_namecache.h>
#include <coda/coda_io.h>

#include "ioconf.h"

#define CTL_C

int coda_psdev_print_entry = 0;
static
int outstanding_upcalls = 0;
int coda_call_sleep = PZERO - 1;
#ifdef	CTL_C
int coda_pcatch = PCATCH;
#else
#endif

int coda_kernel_version = CODA_KERNEL_VERSION;

#define ENTRY if(coda_psdev_print_entry) myprintf(("Entered %s\n",__func__))

dev_type_open(vc_nb_open);
dev_type_close(vc_nb_close);
dev_type_read(vc_nb_read);
dev_type_write(vc_nb_write);
dev_type_ioctl(vc_nb_ioctl);
dev_type_poll(vc_nb_poll);
dev_type_kqfilter(vc_nb_kqfilter);

const struct cdevsw vcoda_cdevsw = {
	.d_open = vc_nb_open,
	.d_close = vc_nb_close,
	.d_read = vc_nb_read,
	.d_write = vc_nb_write,
	.d_ioctl = vc_nb_ioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = vc_nb_poll,
	.d_mmap = nommap,
	.d_kqfilter = vc_nb_kqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER,
};

struct vmsg {
    TAILQ_ENTRY(vmsg) vm_chain;
    void *	 vm_data;
    u_short	 vm_flags;
    u_short      vm_inSize;	/* Size is at most 5000 bytes */
    u_short	 vm_outSize;
    u_short	 vm_opcode; 	/* copied from data to save ptr lookup */
    int		 vm_unique;
    void *	 vm_sleep;	/* Not used by Mach. */
};

struct coda_mntinfo coda_mnttbl[NVCODA];

#define	VM_READ	    1
#define	VM_WRITE    2
#define	VM_INTR	    4

/* vcodaattach: do nothing */
void
vcodaattach(int n)
{
}

/*
 * These functions are written for NetBSD.
 */
int
vc_nb_open(dev_t dev, int flag, int mode,
    struct lwp *l)
{
    struct vcomm *vcp;

    ENTRY;

    if (minor(dev) >= NVCODA)
	return(ENXIO);

    if (!coda_nc_initialized)
	coda_nc_init();

    vcp = &coda_mnttbl[minor(dev)].mi_vcomm;
    if (VC_OPEN(vcp))
	return(EBUSY);

    selinit(&vcp->vc_selproc);
    TAILQ_INIT(&vcp->vc_requests);
    TAILQ_INIT(&vcp->vc_replies);
    MARK_VC_OPEN(vcp);

    coda_mnttbl[minor(dev)].mi_vfsp = NULL;
    coda_mnttbl[minor(dev)].mi_rootvp = NULL;

    return(0);
}

int
vc_nb_close(dev_t dev, int flag, int mode, struct lwp *l)
{
    struct vcomm *vcp;
    struct vmsg *vmp;
    struct coda_mntinfo *mi;
    int                 err;

    ENTRY;

    if (minor(dev) >= NVCODA)
	return(ENXIO);

    mi = &coda_mnttbl[minor(dev)];
    vcp = &(mi->mi_vcomm);

    if (!VC_OPEN(vcp))
	panic("vcclose: not open");

    /* prevent future operations on this vfs from succeeding by auto-
     * unmounting any vfs mounted via this device. This frees user or
     * sysadm from having to remember where all mount points are located.
     * Put this before WAKEUPs to avoid queuing new messages between
     * the WAKEUP and the unmount (which can happen if we're unlucky)
     */
    if (!mi->mi_rootvp) {
	/* just a simple open/close w no mount */
	MARK_VC_CLOSED(vcp);
	return 0;
    }

    /* Let unmount know this is for real */
    VTOC(mi->mi_rootvp)->c_flags |= C_UNMOUNTING;
    coda_unmounting(mi->mi_vfsp);

    /* Wakeup clients so they can return. */
    while ((vmp = TAILQ_FIRST(&vcp->vc_requests)) != NULL) {
	TAILQ_REMOVE(&vcp->vc_requests, vmp, vm_chain);

	/* Free signal request messages and don't wakeup cause
	   no one is waiting. */
	if (vmp->vm_opcode == CODA_SIGNAL) {
	    CODA_FREE(vmp->vm_data, VC_IN_NO_DATA);
	    CODA_FREE(vmp, sizeof(struct vmsg));
	    continue;
	}
	outstanding_upcalls++;
	wakeup(&vmp->vm_sleep);
    }

    while ((vmp = TAILQ_FIRST(&vcp->vc_replies)) != NULL) {
	TAILQ_REMOVE(&vcp->vc_replies, vmp, vm_chain);

	outstanding_upcalls++;
	wakeup(&vmp->vm_sleep);
    }

    MARK_VC_CLOSED(vcp);

    if (outstanding_upcalls) {
#ifdef	CODA_VERBOSE
	printf("presleep: outstanding_upcalls = %d\n", outstanding_upcalls);
    	(void) tsleep(&outstanding_upcalls, coda_call_sleep, "coda_umount", 0);
	printf("postsleep: outstanding_upcalls = %d\n", outstanding_upcalls);
#else
    	(void) tsleep(&outstanding_upcalls, coda_call_sleep, "coda_umount", 0);
#endif
    }

    err = dounmount(mi->mi_vfsp, flag, l);
    if (err)
	myprintf(("Error %d unmounting vfs in vcclose(%llu)\n",
	           err, (unsigned long long)minor(dev)));
    seldestroy(&vcp->vc_selproc);
    return 0;
}

int
vc_nb_read(dev_t dev, struct uio *uiop, int flag)
{
    struct vcomm *	vcp;
    struct vmsg *vmp;
    int error = 0;

    ENTRY;

    if (minor(dev) >= NVCODA)
	return(ENXIO);

    vcp = &coda_mnttbl[minor(dev)].mi_vcomm;

    /* Get message at head of request queue. */
    vmp = TAILQ_FIRST(&vcp->vc_requests);
    if (vmp == NULL)
	return(0);	/* Nothing to read */

    /* Move the input args into userspace */
    uiop->uio_rw = UIO_READ;
    error = uiomove(vmp->vm_data, vmp->vm_inSize, uiop);
    if (error) {
	myprintf(("vcread: error (%d) on uiomove\n", error));
	error = EINVAL;
    }

    TAILQ_REMOVE(&vcp->vc_requests, vmp, vm_chain);

    /* If request was a signal, free up the message and don't
       enqueue it in the reply queue. */
    if (vmp->vm_opcode == CODA_SIGNAL) {
	if (codadebug)
	    myprintf(("vcread: signal msg (%d, %d)\n",
		      vmp->vm_opcode, vmp->vm_unique));
	CODA_FREE(vmp->vm_data, VC_IN_NO_DATA);
	CODA_FREE(vmp, sizeof(struct vmsg));
	return(error);
    }

    vmp->vm_flags |= VM_READ;
    TAILQ_INSERT_TAIL(&vcp->vc_replies, vmp, vm_chain);

    return(error);
}

int
vc_nb_write(dev_t dev, struct uio *uiop, int flag)
{
    struct vcomm *	vcp;
    struct vmsg *vmp;
    struct coda_out_hdr *out;
    u_long seq;
    u_long opcode;
    int tbuf[2];
    int error = 0;

    ENTRY;

    if (minor(dev) >= NVCODA)
	return(ENXIO);

    vcp = &coda_mnttbl[minor(dev)].mi_vcomm;

    /* Peek at the opcode, unique without transfering the data. */
    uiop->uio_rw = UIO_WRITE;
    error = uiomove(tbuf, sizeof(int) * 2, uiop);
    if (error) {
	myprintf(("vcwrite: error (%d) on uiomove\n", error));
	return(EINVAL);
    }

    opcode = tbuf[0];
    seq = tbuf[1];

    if (codadebug)
	myprintf(("vcwrite got a call for %ld.%ld\n", opcode, seq));

    if (DOWNCALL(opcode)) {
	union outputArgs pbuf;

	/* get the rest of the data. */
	uiop->uio_rw = UIO_WRITE;
	error = uiomove(&pbuf.coda_purgeuser.oh.result, sizeof(pbuf) - (sizeof(int)*2), uiop);
	if (error) {
	    myprintf(("vcwrite: error (%d) on uiomove (Op %ld seq %ld)\n",
		      error, opcode, seq));
	    return(EINVAL);
	    }

	return handleDownCall(opcode, &pbuf);
    }

    /* Look for the message on the (waiting for) reply queue. */
    TAILQ_FOREACH(vmp, &vcp->vc_replies, vm_chain) {
	if (vmp->vm_unique == seq) break;
    }

    if (vmp == NULL) {
	if (codadebug)
	    myprintf(("vcwrite: msg (%ld, %ld) not found\n", opcode, seq));

	return(ESRCH);
    }

    /* Remove the message from the reply queue */
    TAILQ_REMOVE(&vcp->vc_replies, vmp, vm_chain);

    /* move data into response buffer. */
    out = (struct coda_out_hdr *)vmp->vm_data;
    /* Don't need to copy opcode and uniquifier. */

    /* get the rest of the data. */
    if (vmp->vm_outSize < uiop->uio_resid) {
	myprintf(("vcwrite: more data than asked for (%d < %lu)\n",
		  vmp->vm_outSize, (unsigned long) uiop->uio_resid));
	wakeup(&vmp->vm_sleep); 	/* Notify caller of the error. */
	return(EINVAL);
    }

    tbuf[0] = uiop->uio_resid; 	/* Save this value. */
    uiop->uio_rw = UIO_WRITE;
    error = uiomove(&out->result, vmp->vm_outSize - (sizeof(int) * 2), uiop);
    if (error) {
	myprintf(("vcwrite: error (%d) on uiomove (op %ld seq %ld)\n",
		  error, opcode, seq));
	return(EINVAL);
    }

    /* I don't think these are used, but just in case. */
    /* XXX - aren't these two already correct? -bnoble */
    out->opcode = opcode;
    out->unique = seq;
    vmp->vm_outSize	= tbuf[0];	/* Amount of data transferred? */
    vmp->vm_flags |= VM_WRITE;
    wakeup(&vmp->vm_sleep);

    return(0);
}

int
vc_nb_ioctl(dev_t dev, u_long cmd, void *addr, int flag,
    struct lwp *l)
{
    ENTRY;

    switch(cmd) {
    case CODARESIZE: {
	struct coda_resize *data = (struct coda_resize *)addr;
	return(coda_nc_resize(data->hashsize, data->heapsize, IS_DOWNCALL));
	break;
    }
    case CODASTATS:
	if (coda_nc_use) {
	    coda_nc_gather_stats();
	    return(0);
	} else {
	    return(ENODEV);
	}
	break;
    case CODAPRINT:
	if (coda_nc_use) {
	    print_coda_nc();
	    return(0);
	} else {
	    return(ENODEV);
	}
	break;
    case CIOC_KERNEL_VERSION:
	switch (*(u_int *)addr) {
	case 0:
		*(u_int *)addr = coda_kernel_version;
		return 0;
		break;
	case 1:
	case 2:
		if (coda_kernel_version != *(u_int *)addr)
		    return ENOENT;
		else
		    return 0;
	default:
		return ENOENT;
	}
    	break;
    default :
	return(EINVAL);
	break;
    }
}

int
vc_nb_poll(dev_t dev, int events, struct lwp *l)
{
    struct vcomm *vcp;
    int event_msk = 0;

    ENTRY;

    if (minor(dev) >= NVCODA)
	return(ENXIO);

    vcp = &coda_mnttbl[minor(dev)].mi_vcomm;

    event_msk = events & (POLLIN|POLLRDNORM);
    if (!event_msk)
	return(0);

    if (!TAILQ_EMPTY(&vcp->vc_requests))
	return(events & (POLLIN|POLLRDNORM));

    selrecord(l, &(vcp->vc_selproc));

    return(0);
}

static void
filt_vc_nb_detach(struct knote *kn)
{
	struct vcomm *vcp = kn->kn_hook;

	SLIST_REMOVE(&vcp->vc_selproc.sel_klist, kn, knote, kn_selnext);
}

static int
filt_vc_nb_read(struct knote *kn, long hint)
{
	struct vcomm *vcp = kn->kn_hook;
	struct vmsg *vmp;

	vmp = TAILQ_FIRST(&vcp->vc_requests);
	if (vmp == NULL)
		return (0);

	kn->kn_data = vmp->vm_inSize;
	return (1);
}

static const struct filterops vc_nb_read_filtops =
	{ 1, NULL, filt_vc_nb_detach, filt_vc_nb_read };

int
vc_nb_kqfilter(dev_t dev, struct knote *kn)
{
	struct vcomm *vcp;
	struct klist *klist;

	ENTRY;

	if (minor(dev) >= NVCODA)
		return(ENXIO);

	vcp = &coda_mnttbl[minor(dev)].mi_vcomm;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &vcp->vc_selproc.sel_klist;
		kn->kn_fop = &vc_nb_read_filtops;
		break;

	default:
		return (EINVAL);
	}

	kn->kn_hook = vcp;

	SLIST_INSERT_HEAD(klist, kn, kn_selnext);

	return (0);
}

/*
 * Statistics
 */
struct coda_clstat coda_clstat;

/*
 * Key question: whether to sleep interruptably or uninterruptably when
 * waiting for Venus.  The former seems better (cause you can ^C a
 * job), but then GNU-EMACS completion breaks. Use tsleep with no
 * timeout, and no longjmp happens. But, when sleeping
 * "uninterruptibly", we don't get told if it returns abnormally
 * (e.g. kill -9).
 */

int
coda_call(struct coda_mntinfo *mntinfo, int inSize, int *outSize,
	void *buffer)
{
	struct vcomm *vcp;
	struct vmsg *vmp;
	int error;
#ifdef	CTL_C
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	sigset_t psig_omask;
	int i;
	psig_omask = l->l_sigmask;	/* XXXSA */
#endif
	if (mntinfo == NULL) {
	    /* Unlikely, but could be a race condition with a dying warden */
	    return ENODEV;
	}

	vcp = &(mntinfo->mi_vcomm);

	coda_clstat.ncalls++;
	coda_clstat.reqs[((struct coda_in_hdr *)buffer)->opcode]++;

	if (!VC_OPEN(vcp))
	    return(ENODEV);

	CODA_ALLOC(vmp,struct vmsg *,sizeof(struct vmsg));
	/* Format the request message. */
	vmp->vm_data = buffer;
	vmp->vm_flags = 0;
	vmp->vm_inSize = inSize;
	vmp->vm_outSize
	    = *outSize ? *outSize : inSize; /* |buffer| >= inSize */
	vmp->vm_opcode = ((struct coda_in_hdr *)buffer)->opcode;
	vmp->vm_unique = ++vcp->vc_seq;
	if (codadebug)
	    myprintf(("Doing a call for %d.%d\n",
		      vmp->vm_opcode, vmp->vm_unique));

	/* Fill in the common input args. */
	((struct coda_in_hdr *)buffer)->unique = vmp->vm_unique;

	/* Append msg to request queue and poke Venus. */
	TAILQ_INSERT_TAIL(&vcp->vc_requests, vmp, vm_chain);
	selnotify(&(vcp->vc_selproc), 0, 0);

	/* We can be interrupted while we wait for Venus to process
	 * our request.  If the interrupt occurs before Venus has read
	 * the request, we dequeue and return. If it occurs after the
	 * read but before the reply, we dequeue, send a signal
	 * message, and return. If it occurs after the reply we ignore
	 * it. In no case do we want to restart the syscall.  If it
	 * was interrupted by a venus shutdown (vcclose), return
	 * ENODEV.  */

	/* Ignore return, We have to check anyway */
#ifdef	CTL_C
	/* This is work in progress.  Setting coda_pcatch lets tsleep reawaken
	   on a ^c or ^z.  The problem is that emacs sets certain interrupts
	   as SA_RESTART.  This means that we should exit sleep handle the
	   "signal" and then go to sleep again.  Mostly this is done by letting
	   the syscall complete and be restarted.  We are not idempotent and
	   can not do this.  A better solution is necessary.
	 */
	i = 0;
	do {
	    error = tsleep(&vmp->vm_sleep, (coda_call_sleep|coda_pcatch), "coda_call", hz*2);
	    if (error == 0)
	    	break;
	    mutex_enter(p->p_lock);
	    if (error == EWOULDBLOCK) {
#ifdef	CODA_VERBOSE
		    printf("coda_call: tsleep TIMEOUT %d sec\n", 2+2*i);
#endif
    	    } else if (sigispending(l, SIGIO)) {
		    sigaddset(&l->l_sigmask, SIGIO);
#ifdef	CODA_VERBOSE
		    printf("coda_call: tsleep returns %d SIGIO, cnt %d\n", error, i);
#endif
    	    } else if (sigispending(l, SIGALRM)) {
		    sigaddset(&l->l_sigmask, SIGALRM);
#ifdef	CODA_VERBOSE
		    printf("coda_call: tsleep returns %d SIGALRM, cnt %d\n", error, i);
#endif
	    } else {
		    sigset_t tmp;
		    tmp = p->p_sigpend.sp_set;	/* array assignment */
		    sigminusset(&l->l_sigmask, &tmp);

#ifdef	CODA_VERBOSE
		    printf("coda_call: tsleep returns %d, cnt %d\n", error, i);
		    printf("coda_call: siglist = %x.%x.%x.%x, sigmask = %x.%x.%x.%x, mask %x.%x.%x.%x\n",
			    p->p_sigpend.sp_set.__bits[0], p->p_sigpend.sp_set.__bits[1],
			    p->p_sigpend.sp_set.__bits[2], p->p_sigpend.sp_set.__bits[3],
			    l->l_sigmask.__bits[0], l->l_sigmask.__bits[1],
			    l->l_sigmask.__bits[2], l->l_sigmask.__bits[3],
			    tmp.__bits[0], tmp.__bits[1], tmp.__bits[2], tmp.__bits[3]);
#endif
		    mutex_exit(p->p_lock);
		    break;
#ifdef	notyet
		    sigminusset(&l->l_sigmask, &p->p_sigpend.sp_set);
		    printf("coda_call: siglist = %x.%x.%x.%x, sigmask = %x.%x.%x.%x\n",
			    p->p_sigpend.sp_set.__bits[0], p->p_sigpend.sp_set.__bits[1],
			    p->p_sigpend.sp_set.__bits[2], p->p_sigpend.sp_set.__bits[3],
			    l->l_sigmask.__bits[0], l->l_sigmask.__bits[1],
			    l->l_sigmask.__bits[2], l->l_sigmask.__bits[3]);
#endif
	    }
	    mutex_exit(p->p_lock);
	} while (error && i++ < 128 && VC_OPEN(vcp));
	l->l_sigmask = psig_omask;	/* XXXSA */
#else
	(void) tsleep(&vmp->vm_sleep, coda_call_sleep, "coda_call", 0);
#endif
	if (VC_OPEN(vcp)) {	/* Venus is still alive */
 	/* Op went through, interrupt or not... */
	    if (vmp->vm_flags & VM_WRITE) {
		error = 0;
		*outSize = vmp->vm_outSize;
	    }

	    else if (!(vmp->vm_flags & VM_READ)) {
		/* Interrupted before venus read it. */
#ifdef	CODA_VERBOSE
		if (1)
#else
		if (codadebug)
#endif
		    myprintf(("interrupted before read: op = %d.%d, flags = %x\n",
			   vmp->vm_opcode, vmp->vm_unique, vmp->vm_flags));

		TAILQ_REMOVE(&vcp->vc_requests, vmp, vm_chain);
		error = EINTR;
	    }

	    else {
		/* (!(vmp->vm_flags & VM_WRITE)) means interrupted after
                   upcall started */
		/* Interrupted after start of upcall, send venus a signal */
		struct coda_in_hdr *dog;
		struct vmsg *svmp;

#ifdef	CODA_VERBOSE
		if (1)
#else
		if (codadebug)
#endif
		    myprintf(("Sending Venus a signal: op = %d.%d, flags = %x\n",
			   vmp->vm_opcode, vmp->vm_unique, vmp->vm_flags));

		TAILQ_REMOVE(&vcp->vc_replies, vmp, vm_chain);
		error = EINTR;

		CODA_ALLOC(svmp, struct vmsg *, sizeof (struct vmsg));

		CODA_ALLOC((svmp->vm_data), char *, sizeof (struct coda_in_hdr));
		dog = (struct coda_in_hdr *)svmp->vm_data;

		svmp->vm_flags = 0;
		dog->opcode = svmp->vm_opcode = CODA_SIGNAL;
		dog->unique = svmp->vm_unique = vmp->vm_unique;
		svmp->vm_inSize = sizeof (struct coda_in_hdr);
/*??? rvb */	svmp->vm_outSize = sizeof (struct coda_in_hdr);

		if (codadebug)
		    myprintf(("coda_call: enqueing signal msg (%d, %d)\n",
			   svmp->vm_opcode, svmp->vm_unique));

		/* insert at head of queue */
		TAILQ_INSERT_HEAD(&vcp->vc_requests, svmp, vm_chain);
		selnotify(&(vcp->vc_selproc), 0, 0);
	    }
	}

	else {	/* If venus died (!VC_OPEN(vcp)) */
	    if (codadebug)
		myprintf(("vcclose woke op %d.%d flags %d\n",
		       vmp->vm_opcode, vmp->vm_unique, vmp->vm_flags));

		error = ENODEV;
	}

	CODA_FREE(vmp, sizeof(struct vmsg));

	if (outstanding_upcalls > 0 && (--outstanding_upcalls == 0))
		wakeup(&outstanding_upcalls);

	if (!error)
		error = ((struct coda_out_hdr *)buffer)->result;
	return(error);
}

MODULE(MODULE_CLASS_DRIVER, vcoda, NULL);

static int
vcoda_modcmd(modcmd_t cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
	{
		int cmajor, dmajor;
		vcodaattach(NVCODA);

		dmajor = cmajor = -1;
		return devsw_attach("vcoda", NULL, &dmajor,
		    &vcoda_cdevsw, &cmajor);
	}
#endif
		break;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		{
			for  (size_t i = 0; i < NVCODA; i++) {
				struct vcomm *vcp = &coda_mnttbl[i].mi_vcomm;
				if (VC_OPEN(vcp))
					return EBUSY;
			}
			return devsw_detach(NULL, &vcoda_cdevsw);
		}
#endif
		break;

	case MODULE_CMD_STAT:
		return ENOTTY;

	default:
		return ENOTTY;
	}
	return error;
}
