/*	$NetBSD: verified_exec.c,v 1.4 2003/07/14 15:47:04 lukem Exp $	*/

/*-
 * Copyright (c) 1998-1999 Brett Lymn
 *                         (blymn@baea.com.au, brett_lymn@yahoo.com.au)
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: verified_exec.c,v 1.4 2003/07/14 15:47:04 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/verified_exec.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/lock.h>
#include <sys/queue.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/namei.h>

struct verified_exec_softc {
	struct device	veriexec_dev;
};

const struct cdevsw verifiedexec_cdevsw = {
        verifiedexecopen, verifiedexecclose, noread, nowrite,
	verifiedexecioctl, nostop, notty, nopoll, nommap, nokqfilter,
};

/* internal structures */
LIST_HEAD(veriexec_devhead, veriexec_dev_list) veriexec_dev_head;
/*LIST_HEAD(veriexec_file_devhead, veriexec_dev_list) veriexec_file_dev_head;*/
struct veriexec_devhead veriexec_file_dev_head;

/* Autoconfiguration glue */
void	verifiedexecattach(struct device *parent, struct device *self,
			 void *aux);
int     verifiedexecopen(dev_t dev, int flags, int fmt, struct proc *p);
int     verifiedexecclose(dev_t dev, int flags, int fmt, struct proc *p);
int     verifiedexecioctl(dev_t dev, u_long cmd, caddr_t data, int flags,
			struct proc *p);
void    add_veriexec_inode(struct veriexec_dev_list *list, unsigned long inode,
			unsigned char fingerprint[MAXFINGERPRINTLEN],
			unsigned char type, unsigned char fp_type);
struct veriexec_dev_list *find_veriexec_dev(unsigned long dev,
				      struct veriexec_devhead *head);

/*
 * Attach for autoconfig to find.  Initialise the lists and return...
 */
void
verifiedexecattach(struct device *parent, struct device *self, void *aux)
{
	LIST_INIT(&veriexec_dev_head);
	LIST_INIT(&veriexec_file_dev_head);
}

int
verifiedexecopen(dev_t dev, int flags, int fmt, struct proc *p)
{
	return 0;
}

int
verifiedexecclose(dev_t dev, int flags, int fmt, struct proc *p)
{
#ifdef VERIFIED_EXEC_DEBUG_VERBOSE
	struct veriexec_dev_list *lp;
	struct veriexec_inode_list *ip;

	printf("Loaded exec fingerprint list is:\n");
	for (lp = LIST_FIRST(&veriexec_dev_head); lp != NULL;
	     lp = LIST_NEXT(lp, entries)) {
		for (ip = LIST_FIRST(&(lp->inode_head)); ip != NULL;
		     ip = LIST_NEXT(ip, entries)) {
			printf("Got loaded fingerprint for dev %lu, inode %lu\n",
			       lp->id, ip->inode);
		}
	}

	printf("\n\nLoaded file fingerprint list is:\n");
	for (lp = LIST_FIRST(&veriexec_file_dev_head); lp != NULL;
	     lp = LIST_NEXT(lp, entries)) {
		for (ip = LIST_FIRST(&(lp->inode_head)); ip != NULL;
		     ip = LIST_NEXT(ip, entries)) {
			printf("Got loaded fingerprint for dev %lu, inode %lu\n",
			       lp->id, ip->inode);
		}
	}
#endif
	return 0;
}

/*
 * Search the list of devices looking for the one given.  If it is not
 * in the list then add it.
 */
struct veriexec_dev_list *
find_veriexec_dev(unsigned long dev, struct veriexec_devhead *head)
{
	struct veriexec_dev_list *lp;

	for (lp = LIST_FIRST(head); lp != NULL;
	     lp = LIST_NEXT(lp, entries))
		if (lp->id == dev) break;

	if (lp == NULL) {
		  /* if pointer is null then entry not there, add a new one */
		MALLOC(lp, struct veriexec_dev_list *,
                       sizeof(struct veriexec_dev_list), M_TEMP, M_WAITOK);
		LIST_INIT(&(lp->inode_head));
		lp->id = dev;
		LIST_INSERT_HEAD(head, lp, entries);
	}

	return lp;
}

/*
 * Add a file's inode and fingerprint to the list of inodes attached
 * to the device id.  Only add the entry if it is not already on the
 * list.
 */
void
add_veriexec_inode(struct veriexec_dev_list *list, unsigned long inode,
		unsigned char fingerprint[MAXFINGERPRINTLEN],
		unsigned char type, unsigned char fp_type)
{
	struct veriexec_inode_list *ip;

	for (ip = LIST_FIRST(&(list->inode_head)); ip != NULL;
	     ip = LIST_NEXT(ip, entries))
		  /* check for a dupe inode in the list, skip if an entry
		   * exists for this inode except for when the type is
		   * VERIEXEC_INDIRECT, always set the type when it is so
		   * we don't get a hole caused by conflicting types on
		   * hardlinked files.  XXX maybe we should validate
		   * fingerprint is same and complain if it is not...
		   */
		if (ip->inode == inode) {
			if (type == VERIEXEC_INDIRECT)
				ip->type = type;
			return;
		}
	

	MALLOC(ip, struct veriexec_inode_list *, sizeof(struct veriexec_inode_list),
	       M_TEMP, M_WAITOK);
	ip->type = type;
	ip->fp_type = fp_type;
	ip->inode = inode;
	memcpy(ip->fingerprint, fingerprint, sizeof(ip->fingerprint));
	LIST_INSERT_HEAD(&(list->inode_head), ip, entries);
}

/*
 * Handle the ioctl for the device
 */
int
verifiedexecioctl(dev_t dev, u_long cmd, caddr_t data, int flags,
		struct proc *p)
{
	int error = 0;
	struct verified_exec_params *params = (struct verified_exec_params *)data;
	struct nameidata nid;
	struct vattr vattr;
	struct veriexec_dev_list *dlp;

#ifdef VERIFIED_EXEC_DEBUG
	printf("veriexec_ioctl: got cmd 0x%lx for file %s\n", cmd, params->file);
#endif
	
	switch (cmd) {
	  case VERIEXECLOAD:
		if (securelevel > 0) {
			  /* don't allow updates when secure */
			error = EPERM;
		} else {
			  /*
			   * Get the attributes for the file name passed
			   * stash the file's device id and inode number
			   * along with it's fingerprint in a list for
			   * exec to use later.
			   */
                        NDINIT(&nid, LOOKUP, FOLLOW, UIO_USERSPACE,
                               params->file, p);
			if ((error = vn_open(&nid, FREAD, 0)) != 0) {
				return(error);
			}
			error = VOP_GETATTR(nid.ni_vp, &vattr, p->p_ucred, p);
			if (error) {
				nid.ni_vp->fp_status = FINGERPRINT_INVALID;
				VOP_UNLOCK(nid.ni_vp, 0);
				(void) vn_close(nid.ni_vp, FREAD,
						p->p_ucred, p);
				return(error);
			}
			/* invalidate the node fingerprint status
			 * which will have been set in the vn_open
			 * and would always be FINGERPRINT_NOTFOUND
			 */
			nid.ni_vp->fp_status = FINGERPRINT_INVALID;
			VOP_UNLOCK(nid.ni_vp, 0);
			(void) vn_close(nid.ni_vp, FREAD, p->p_ucred, p);
			  /* vattr.va_fsid = dev, vattr.va_fileid = inode */
			if (params->type == VERIEXEC_FILE) {
				dlp = find_veriexec_dev(vattr.va_fsid,
						     &veriexec_file_dev_head);
			} else {
				dlp = find_veriexec_dev(vattr.va_fsid,
						     &veriexec_dev_head);
			}
			
			add_veriexec_inode(dlp, vattr.va_fileid,
					params->fingerprint, params->type,
					params->fp_type);
		}
		break;

	default:
		error = ENODEV;
	}

	return (error);
}

