/*	$NetBSD: kern_verifiedexec.c,v 1.3 2003/04/01 01:41:39 thorpej Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Brett Lymn and Jason R Fink
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/mount.h> 
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/md5.h>
#include <sys/sha1.h>
#include <sys/verified_exec.h>

/* Set the buffer to a single page for md5 and sha1 */
#define BUF_SIZE PAGE_SIZE

extern LIST_HEAD(veriexec_devhead, veriexec_dev_list) veriexec_dev_head;

static int
md5_fingerprint(struct vnode *vp, struct veriexec_inode_list *ip,
		struct proc *p, u_quad_t file_size, char *fingerprint);

static int
sha1_fingerprint(struct vnode *vp, struct veriexec_inode_list *ip,
		struct proc *p, u_quad_t file_size, char *fingerprint);


/*
 * md5_fingerprint:
 *   Evaluate the md5 fingerprint of the given file.
 *
 */
static int
md5_fingerprint(struct vnode *vp, struct veriexec_inode_list *ip,
		struct proc *p, u_quad_t file_size, char *fingerprint)
{
        u_quad_t        j;
        MD5_CTX         md5context;
	char            *filebuf;
	size_t          resid;
	int             count, error;
	
	filebuf = malloc(BUF_SIZE, M_TEMP, M_WAITOK);
	MD5Init(&md5context);
	
	for (j = 0; j < file_size; j+= BUF_SIZE) {
		if ((j + BUF_SIZE) > file_size) {
			count = file_size - j;
		} else
			count = BUF_SIZE;

		error = vn_rdwr(UIO_READ, vp, filebuf, count, j,
                                UIO_SYSSPACE, 0, p->p_ucred, &resid, p);

		if (error) {
			free(filebuf, M_TEMP);
			return error;
		}
		
		MD5Update(&md5context, filebuf, (unsigned int) count);

	}

	MD5Final(fingerprint, &md5context);
	free(filebuf, M_TEMP);
	return 0;
}

static int
sha1_fingerprint(struct vnode *vp, struct veriexec_inode_list *ip,
		 struct proc *p, u_quad_t file_size, char *fingerprint)
{
        u_quad_t        j;
        SHA1_CTX        sha1context;
	char            *filebuf;
	size_t          resid;
	int             count, error;

	filebuf = malloc(BUF_SIZE, M_TEMP, M_WAITOK);
	SHA1Init(&sha1context);

	for (j = 0; j < file_size; j+= BUF_SIZE) {
		if ((j + BUF_SIZE) > file_size) {
			count = file_size - j;
		} else
			count = BUF_SIZE;

		error = vn_rdwr(UIO_READ, vp, filebuf, count, j,
				UIO_SYSSPACE, 0, p->p_ucred, &resid, p);

		if (error) {
			free(filebuf, M_TEMP);
			return error;
		}

		SHA1Update(&sha1context, filebuf, (unsigned int) count);

	}

	SHA1Final(fingerprint, &sha1context);
	free(filebuf, M_TEMP);
	return 0;
}

/*
 * evaluate_fingerprint:
 *   Check the fingerprint type for the given file and evaluate the
 * fingerprint for that file.  It is assumed that fingerprint has sufficient
 * storage to hold the resulting fingerprint string.
 *
 */
int
evaluate_fingerprint(struct vnode *vp, struct veriexec_inode_list *ip,
		     struct proc *p, u_quad_t file_size, char *fingerprint)
{
	int error;
	
	switch (ip->fp_type) {
	case FINGERPRINT_TYPE_MD5:
		error = md5_fingerprint(vp, ip, p, file_size, fingerprint);
		break;

	case FINGERPRINT_TYPE_SHA1:
		error = sha1_fingerprint(vp, ip, p, file_size, fingerprint);
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}

/*
 * fingerprintcmp:
 *    Compare the two given fingerprints to see if they are the same
 * Differing fingerprint methods may have differing lengths which
 * is handled by this routine.  This function follows the convention
 * of other cmp functions and returns 0 if the fingerprints match and
 * 1 if they don't.
 */
int
fingerprintcmp(struct veriexec_inode_list *ip, unsigned char *digest)
{
	switch(ip->fp_type) {
	case FINGERPRINT_TYPE_MD5:
		return memcmp(ip->fingerprint, digest, MD5_FINGERPRINTLEN);
		break;

	case FINGERPRINT_TYPE_SHA1:
		return memcmp(ip->fingerprint, digest, SHA1_FINGERPRINTLEN);
		break;

	default:
		  /* unknown fingerprint type, just fail it after whining */
		printf("fingerprintcmp: unknown fingerprint type\n");
		return 1;
		break;
	}
}


/*
 * get_veriexec_inode:
 *   Search the given verified exec fingerprint list for the given
 * fileid.  If it exists then return a pointer to the list entry,
 * otherwise return NULL.  If found_dev is non-NULL set this to true
 * iff the given device has a fingerprint list.
 *
 */
struct veriexec_inode_list *
get_veriexec_inode(struct veriexec_devhead *head, long fsid, long fileid,
		char *found_dev)
{
	struct 	veriexec_dev_list 	 *lp;
        struct 	veriexec_inode_list *ip;

	ip = NULL;
	if (found_dev != NULL)
		*found_dev = 0;
	
#ifdef VERIFIED_EXEC_DEBUG
	printf("searching for file %lu on device %lu\n", fileid, fsid);
#endif
	
	for (lp = LIST_FIRST(head); lp != NULL; lp = LIST_NEXT(lp, entries))
		if (lp->id == fsid)
			break;

	if (lp != NULL) {
#ifdef VERIFIED_EXEC_DEBUG
		printf("found matching dev number %lu\n", lp->id);
#endif
		if (found_dev != NULL)
			*found_dev = 1;
		
		for (ip = LIST_FIRST(&(lp->inode_head)); ip != NULL;
		     ip = LIST_NEXT(ip, entries))
			if (ip->inode == fileid)
				break;
	}

	return ip;
}

/*
 * check veriexec:
 *   check a file signature and return a status to check_exec.
 */
int
check_veriexec(struct proc *p, struct vnode *vp, struct exec_package *epp,
		int direct_exec)
{
        int             error;
        char            digest[MAXFINGERPRINTLEN], found_dev;
        struct 	veriexec_inode_list *ip;

	error = 0;
	found_dev = 0;
        if (vp->fp_status == FINGERPRINT_INVALID) {

#ifdef VERIFIED_EXEC_DEBUG
		printf("looking for loaded signature\n");
#endif
		ip = get_veriexec_inode(&veriexec_dev_head, epp->ep_vap->va_fsid,
				     epp->ep_vap->va_fileid, &found_dev);

		if (found_dev == 0) {
#ifdef VERIFIED_EXEC_DEBUG
			printf("No device entry found\n");
#endif
			vp->fp_status = FINGERPRINT_NODEV;
		}
 
		if (ip != NULL) {
#ifdef VERIFIED_EXEC_DEBUG
			printf("found matching inode number %lu\n", ip->inode);
#endif
			error = evaluate_fingerprint(vp, ip, p,
						     epp->ep_vap->va_size,
					     	     digest);
			if (error)
				return error;

			if (fingerprintcmp(ip, digest) == 0) {
				if (ip->type == VERIEXEC_DIRECT) {
#ifdef VERIFIED_EXEC_DEBUG
					printf("Evaluated fingerprint matches\n");
#endif
					vp->fp_status =	FINGERPRINT_VALID;
				} else {
#ifdef VERIFIED_EXEC_DEBUG
					printf("Evaluated indirect fingerprint matches\n");
#endif
					vp->fp_status =	FINGERPRINT_INDIRECT;
				}
			} else {
#ifdef VERIFIED_EXEC_DEBUG
				printf("Evaluated fingerprint match failed\n");
#endif
				vp->fp_status = FINGERPRINT_NOMATCH;
			}
		} else {
#ifdef VERIFIED_EXEC_DEBUG
			printf("No fingerprint entry found\n");
#endif
			vp->fp_status = FINGERPRINT_NOENTRY;
		}
	}

        switch (vp->fp_status) {
          case FINGERPRINT_INVALID: /* should not happen */
                  printf("Got unexpected FINGERPRINT_INVALID!!!\n");
                  error = EPERM;
                  break; 
          case FINGERPRINT_VALID: /* is ok - report so if debug is on */
#ifdef VERIFIED_EXEC_DEBUG
                  printf("Fingerprint matches\n");
#endif
                  break;

          case FINGERPRINT_INDIRECT: /* fingerprint ok but need to check
                                        for direct execution */
                  if (direct_exec == 1) {
                          printf("Attempt to execute %s (dev %lu, inode %lu) dir
ectly by pid %u (ppid %u, gppid %u)\n",
                                 epp->ep_name, epp->ep_vap->va_fsid,
                                 epp->ep_vap->va_fileid, p->p_pid,
                                 p->p_pptr->p_pid, p->p_pptr->p_pptr->p_pid);
                          if (securelevel > 1)
                                  error = EPERM;
                  }
                  break;

          case FINGERPRINT_NOMATCH: /* does not match - bitch about it */
                  printf("Fingerprint for %s (dev %lu, inode %lu) does not match
 loaded value\n",
                         epp->ep_name, epp->ep_vap->va_fsid,
                         epp->ep_vap->va_fileid);
                  if (securelevel > 1)
                          error = EPERM;
                  break;

          case FINGERPRINT_NOENTRY: /* no entry in the list, complain */
                  printf("No fingerprint for %s (dev %lu, inode %lu)\n",
                         epp->ep_name, epp->ep_vap->va_fsid,
                         epp->ep_vap->va_fileid);
                  if (securelevel > 1)
                          error = EPERM;
                  break;

          case FINGERPRINT_NODEV: /* no signatures for the device, complain */
#ifdef VERIFIED_EXEC_DEBUG
                  printf("No signatures for device %lu\n",
                         epp->ep_vap->va_fsid);
#endif
                  if (securelevel > 1)
			  error = EPERM;
                  break;

          default: /* this should never happen. */
                  printf("Invalid fp_status field for vnode %p\n", vp);
                  error = EPERM;
        }

	return error; 

}
