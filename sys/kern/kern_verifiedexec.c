/*	$NetBSD: kern_verifiedexec.c,v 1.11 2005/04/23 09:10:47 blymn Exp $	*/

/*-
 * Copyright 2005 Elad Efrat <elad@bsd.org.il>
 * Copyright 2005 Brett Lymn <blymn@netbsd.org>
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Brett Lymn and Elad Efrat
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Neither the name of The NetBSD Foundation nor the names of its
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_verifiedexec.c,v 1.11 2005/04/23 09:10:47 blymn Exp $");

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/verified_exec.h>
#if defined(__FreeBSD__)
# include <sys/systm.h>
# include <sys/imgact.h>
# include <crypto/sha1.h>
#else
# include <sys/sha1.h>
#endif
#include "crypto/sha2/sha2.h"
#include "crypto/ripemd160/rmd160.h"
#include <sys/md5.h>

/*int security_veriexec = 0;*/
int security_veriexec_verbose = 0;
int security_veriexec_strict = 0;

char *veriexec_fp_names;
int veriexec_name_max;

/* prototypes */
static void
veriexec_add_fp_name(char *name);

/* Veriexecs table of hash types and their associated information. */
LIST_HEAD(veriexec_ops_head, veriexec_fp_ops) veriexec_ops_list;

struct veriexec_fp_ops veriexec_default_ops[] = {
#ifdef VERIFIED_EXEC_FP_RMD160
	{ "RMD160", RMD160_DIGEST_LENGTH, sizeof(RMD160_CTX),
	  (VERIEXEC_INIT_FN) RMD160Init,
	  (VERIEXEC_UPDATE_FN) RMD160Update,
	  (VERIEXEC_FINAL_FN) RMD160Final, {NULL, NULL}},
#endif

#ifdef VERIFIED_EXEC_FP_SHA256
	{ "SHA256", SHA256_DIGEST_LENGTH, sizeof(SHA256_CTX),
	  (VERIEXEC_INIT_FN) SHA256_Init,
	  (VERIEXEC_UPDATE_FN) SHA256_Update,
	  (VERIEXEC_FINAL_FN) SHA256_Final, {NULL, NULL}},
#endif
	
#ifdef VERIFIED_EXEC_FP_SHA384
	{ "SHA384", SHA384_DIGEST_LENGTH, sizeof(SHA384_CTX),
	  (VERIEXEC_INIT_FN) SHA384_Init,
	  (VERIEXEC_UPDATE_FN) SHA384_Update,
	  (VERIEXEC_FINAL_FN) SHA384_Final, {NULL, NULL}},
#endif
	
#ifdef VERIFIED_EXEC_FP_SHA512
	{ "SHA512", SHA512_DIGEST_LENGTH, sizeof(SHA512_CTX),
	  (VERIEXEC_INIT_FN) SHA512_Init,
	  (VERIEXEC_UPDATE_FN) SHA512_Update,
	  (VERIEXEC_FINAL_FN) SHA512_Final, {NULL, NULL}},
#endif
	
#ifdef VERIFIED_EXEC_FP_SHA1
	{ "SHA1", SHA1_DIGEST_LENGTH, sizeof(SHA1_CTX),
	  (VERIEXEC_INIT_FN) SHA1Init,
	  (VERIEXEC_UPDATE_FN) SHA1Update, (VERIEXEC_FINAL_FN) SHA1Final,
	  {NULL, NULL}},
#endif
	
#ifdef VERIFIED_EXEC_FP_MD5
	{ "MD5", MD5_DIGEST_LENGTH, sizeof(MD5_CTX),
	  (VERIEXEC_INIT_FN) MD5Init,
	  (VERIEXEC_UPDATE_FN) MD5Update, (VERIEXEC_FINAL_FN) MD5Final,
	  {NULL, NULL}},
#endif
};

static unsigned default_ops_count =
        sizeof(veriexec_default_ops) / sizeof(struct veriexec_fp_ops);

#define	VERIEXEC_BUFSIZE	PAGE_SIZE

/*
 * Add fingerprint names to the global list.
 */
static void
veriexec_add_fp_name(char *name)
{
	char *newp;
	unsigned new_max;

	if ((strlen(veriexec_fp_names) + VERIEXEC_TYPE_MAXLEN + 1) >=
	    veriexec_name_max) {
		new_max = veriexec_name_max + 4 * (VERIEXEC_TYPE_MAXLEN + 1);
		if ((newp = realloc(veriexec_fp_names, new_max,
				    M_TEMP, M_WAITOK)) == NULL) {
			printf("veriexec: cannot grow storage to add new "
			      "fingerprint name to name list.  Not adding\n");
			return;
		}

		veriexec_fp_names = newp;
		veriexec_name_max = new_max;
	}

	strlcat(veriexec_fp_names, name, veriexec_name_max);
	strlcat(veriexec_fp_names, " ", veriexec_name_max);
}

/*
 * Initialise the internal "default" fingerprint ops vector list.
 */
void
veriexec_init_fp_ops(void)
{
	unsigned int i;

	veriexec_name_max = default_ops_count * (VERIEXEC_TYPE_MAXLEN + 1) + 1;
	veriexec_fp_names = malloc(veriexec_name_max, M_TEMP, M_WAITOK);
	veriexec_fp_names[0] = '\0';
	
	LIST_INIT(&veriexec_ops_list);

	for (i = 0; i < default_ops_count; i++) {
		LIST_INSERT_HEAD(&veriexec_ops_list, &veriexec_default_ops[i],
				 entries);
		veriexec_add_fp_name(veriexec_default_ops[i].type);
	}
}

struct veriexec_fp_ops *
veriexec_find_ops(u_char *name)
{
	struct veriexec_fp_ops *ops;

	name[VERIEXEC_TYPE_MAXLEN] = '\0';
	
	LIST_FOREACH(ops, &veriexec_ops_list, entries) {
		if ((strlen(name) == strlen(ops->type)) &&
		    (strncasecmp(name, ops->type, sizeof(ops->type) - 1)
		     == 0))
			return (ops);
	}

	return (NULL);
}


/*
 * Calculate fingerprint. Information on hash length and routines used is
 * extracted from veriexec_hash_list according to the hash type.
 */
int
veriexec_fp_calc(struct proc *p, struct vnode *vp,
		 struct veriexec_hash_entry *vhe, uint64_t size, u_char *fp)
{
	void *ctx = NULL;
	u_char *buf = NULL;
	off_t offset, len;
	size_t resid;
	int error = 0;

	/* XXX: This should not happen. Print more details? */
	if (vhe->ops == NULL) {
		panic("Veriexec: Operations vector NULL\n");
	}

	bzero(fp, vhe->ops->hash_len);


	ctx = (void *) malloc(vhe->ops->context_size, M_TEMP, M_WAITOK);
	buf = (u_char *) malloc(VERIEXEC_BUFSIZE, M_TEMP, M_WAITOK);

	(vhe->ops->init)(ctx); /* init the fingerprint context */

	/*
	 * The vnode is locked. sys_execve() does it for us; We have our
	 * own locking in vn_open().
	 */
	for (offset = 0; offset < size; offset += VERIEXEC_BUFSIZE) {
		len = ((size - offset) < VERIEXEC_BUFSIZE) ? (size - offset)
			: VERIEXEC_BUFSIZE;

		error = vn_rdwr(UIO_READ, vp, buf, len, offset, 
				UIO_SYSSPACE,
#ifdef __FreeBSD__
				IO_NODELOCKED,
#else
				0,
#endif
				p->p_ucred, &resid, NULL);

		if (error)
			goto bad;

		  /* calculate fingerprint for each chunk */
		(vhe->ops->update)(ctx, buf, (unsigned int) len);
	}

	  /* finalise the fingerprint calculation */
	(vhe->ops->final)(fp, ctx);

bad:
	free(ctx, M_TEMP);
	free(buf, M_TEMP);

	return (error);
}
	
/* Compare two fingerprints of the same type. */
int
veriexec_fp_cmp(struct veriexec_hash_entry *vhe, u_char *digest)
{
#ifdef VERIFIED_EXEC_DEBUG
	int i;

	if (security_veriexec_verbose > 1) {
		printf("comparing hashes...\n");
		printf("vhe->fp: ");
		for (i = 0; i < vhe->ops->hash_len; i++) {
			printf("%x", vhe->fp[i]);
		}
		printf("\ndigest: ");
		for (i = 0; i < vhe->ops->hash_len; i++) {
			printf("%x", digest[i]);
		}
		printf("\n");
	}
#endif

	return (memcmp(vhe->fp, digest, vhe->ops->hash_len));
}

/* Get the hash table for the specified device. */
struct veriexec_hashtbl *
veriexec_tblfind(dev_t device) {
	struct veriexec_hashtbl *tbl;

	LIST_FOREACH(tbl, &veriexec_tables, hash_list) {
		if (tbl->hash_dev == device)
			return (tbl);
	}

	return (NULL);
}

/* Perform a lookup on a hash table. */
struct veriexec_hash_entry *
veriexec_lookup(dev_t device, ino_t inode)
{
	struct veriexec_hashtbl *tbl;
	struct veriexec_hashhead *tble;
	struct veriexec_hash_entry *e;
	size_t indx;

	tbl = veriexec_tblfind(device);
	if (tbl == NULL)
		return (NULL);

	indx = VERIEXEC_HASH(tbl, inode);
	tble = &(tbl->hash_tbl[indx & VERIEXEC_HASH_MASK(tbl)]);

	LIST_FOREACH(e, tble, entries) {
		if ((e != NULL) && (e->inode == inode))
			return (e);
	}

	return (NULL);
}

/*
 * Add an entry to a hash table. If a collision is found, handle it.
 * The passed entry is allocated in kernel memory.
 */
int
veriexec_hashadd(struct veriexec_hashtbl *tbl, struct veriexec_hash_entry *e)
{
	struct veriexec_hashhead *vhh;
	size_t indx;

	if (tbl == NULL)
		return (EFAULT);

	indx = VERIEXEC_HASH(tbl, e->inode);
	vhh = &(tbl->hash_tbl[indx]);

	if (vhh == NULL)
		panic("Veriexec: veriexec_hashadd: vhh is NULL.");

	LIST_INSERT_HEAD(vhh, e, entries);

	return (0);
}

/*
 * Verify the fingerprint of the given file. If we're called directly from
 * sys_execve(), 'flag' will be VERIEXEC_DIRECT. If we're called from
 * exec_script(), 'flag' will be VERIEXEC_INDIRECT.  If we are called from
 * vn_open(), 'flag' will be VERIEXEC_FILE.
 */
int
veriexec_verify(struct proc *p, struct vnode *vp, struct vattr *va,
		const u_char *name, int flag)
{
	struct veriexec_hash_entry *vhe;
        u_char *digest;
        int error = 0;

	/* Evaluate fingerprint if needed and set the status on the vp. */
	if (vp->fp_status == FINGERPRINT_NOTEVAL) {
		vhe = veriexec_lookup(va->va_fsid, va->va_fileid);
		if (vhe == NULL) {
			vp->fp_status = FINGERPRINT_NOENTRY;
			goto out;
		}
 
		veriexec_dprintf(("Veriexec: veriexec_verify: Got entry for "
				  "%s. (dev=%d, inode=%u)\n", name,
				  va->va_fsid, va->va_fileid));

		/* Calculate fingerprint for the inode. */
		digest = (u_char *) malloc(vhe->ops->hash_len, M_TEMP,
					   M_WAITOK);
		error = veriexec_fp_calc(p, vp, vhe, va->va_size, digest);
		
		if (error) {
			veriexec_dprintf(("Veriexec: veriexec_verify: "
					  "Calculation error.\n"));
			free(digest, M_TEMP);
			return (error);
		}

		if (veriexec_fp_cmp(vhe, digest) == 0) {
			if (vhe->type == VERIEXEC_INDIRECT) {
				vp->fp_status = FINGERPRINT_INDIRECT;
			} else {
				vp->fp_status = FINGERPRINT_VALID;
			}
		} else {
			vp->fp_status = FINGERPRINT_NOMATCH;
		}
		free(digest, M_TEMP);
	}

out:
        switch (vp->fp_status) {
	case FINGERPRINT_NOTEVAL:
		/* Should not happen. */
		panic("Veriexec: Not-evaluated status post-evaluation. "
		      "Inconsistency detected. Report a bug.");

	case FINGERPRINT_VALID:
		/* Valid fingerprint. */
		if ((securelevel >= 1) && security_veriexec_verbose)
			printf("Veriexec: veriexec_verify: Fingerprint "
			       "matches. (file=%s, dev=%ld, inode=%lu)\n",
			       name, va->va_fsid, va->va_fileid);
		break;

	case FINGERPRINT_INDIRECT:
		/* Fingerprint is okay; Make sure it's indirect execution. */
		if (flag == VERIEXEC_DIRECT) {
			printf("Veriexec: Attempt to execute %s "
			       "(dev=%ld, inode=%lu) directly by uid=%u "
			       "(pid=%u, ppid=%u, gppid=%u)\n", name,
			       va->va_fsid, va->va_fileid,
			       p->p_ucred->cr_uid, p->p_pid, 
			       p->p_pptr->p_pid, p->p_pptr->p_pptr->p_pid);

			error = EPERM;
		}

		if ((securelevel >= 1) && security_veriexec_verbose)
			printf("Veriexec: veriexec_verify: Fingerprint "
			       "matches on indirect. (file=%s, dev=%ld, "
			       "inode=%lu)\n",
			       name, va->va_fsid, va->va_fileid);
		break;

	case FINGERPRINT_NOMATCH:
		/* Fingerprint mismatch. Deny execution. */
		printf("Veriexec: Fingerprint mismatch for %s "
		       "(dev=%ld, inode=%lu). Execution "
		       "attempt by uid=%u, pid=%u.\n", name,
		       va->va_fsid, va->va_fileid, 
		       p->p_ucred->cr_uid, p->p_pid);

		if (securelevel >= 2)
			error = EPERM;
		break;

	case FINGERPRINT_NOENTRY:
		/* No entry in the list. */
		if (securelevel >= 1) {
			if (security_veriexec_verbose)
				printf("Veriexec: veriexec_verify: No "
				       "fingerprint for %s (dev=%ld, "
				       "inode=%lu)\n", name, va->va_fsid,
				       va->va_fileid);

			  /*
			   * We only really want to reject file opens on
			   * non-fingerprinted files when we are doing
			   * strict checking.
			   */
			if (((securelevel >= 2) && (flag != VERIEXEC_FILE))
			    || security_veriexec_strict)
				error = EPERM;
		}

		break;

	default:
		/*
		 * Should never happen.
		 * XXX: Print vnode/process?
		 */
		panic("Veriexec: Invalid status post-evaluation in "
		      "veriexec_verify(). Report a bug. (vnode=%p, pid=%u)",
		      vp, p->p_pid);
        }

	return (error);
}

/*
 * Veriexec remove policy code. If we have an entry for the file in our
 * tables, we disallow removing if the securelevel is high or we're in
 * strict mode.
 */
int
veriexec_removechk(struct proc *p, struct vnode *vp, const char *pathbuf)
{
	struct veriexec_hashtbl *tbl;
	struct veriexec_hash_entry *vhe = NULL;
	struct vattr va;
	int error;

	error = VOP_GETATTR(vp, &va, p->p_ucred, p);
	if (error)
		return (error);

	switch (vp->fp_status) {
	case FINGERPRINT_VALID:
	case FINGERPRINT_INDIRECT:
	case FINGERPRINT_NOMATCH:
		if ((securelevel >= 2) || security_veriexec_strict) {
			printf("Veriexec: Denying unlink request for %s "
			       "from uid=%u: File in fingerprint tables. "
			       "(pid=%u, dev=%ld, inode=%lu)\n", pathbuf,
			       p->p_ucred->cr_uid, p->p_pid,
			       va.va_fsid, va.va_fileid);

			error = EPERM;
		} else {
			if (security_veriexec_verbose) {
				printf("Veriexec: veriexec_removechk: Removing"
				       " entry from Veriexec table. (file=%s, "
				       "dev=%ld, inode=%lu)\n", pathbuf,
				       va.va_fsid, va.va_fileid);
			}

			goto veriexec_rm;
		}

		break;

	case FINGERPRINT_NOENTRY:
		return(error);
		break;

	case FINGERPRINT_NOTEVAL:
		/*
		 * Could be we don't have an entry for this, but we can't
		 * risk an unevaluated file or vnode cache flush.
		 */
		vhe = veriexec_lookup(va.va_fsid, va.va_fileid);
		if (vhe == NULL) {
			break;
		}

		if (securelevel >= 2) {
			printf("Veriexec: Denying unlink request for %s from"
			       " uid=%u: File in fingerprint tables. (pid=%u, "
			       "dev=%ld, inode=%lu)\n", pathbuf,
			       p->p_ucred->cr_uid, p->p_pid,
			       va.va_fsid, va.va_fileid);

			error = EPERM;
		} else {
			goto veriexec_rm;
		}

		break;

	default:
		panic("Veriexec: inconsistency in verified exec state"
		      "data");
		break;
		
	}

	return (error);

veriexec_rm:
	tbl = veriexec_tblfind(va.va_fsid);
	if (tbl == NULL) {
		panic("Veriexec: Inconsistency: Could not get table for file"
		      " in lists. Report a bug.");
	}

	tbl->hash_size--;
	LIST_REMOVE(vhe, entries);
	free(vhe->fp, M_TEMP);
	free(vhe, M_TEMP);

	return (error);
}
