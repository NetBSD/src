/*	$NetBSD: kern_verifiedexec.c,v 1.48.6.1 2006/04/22 11:39:59 simonb Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: kern_verifiedexec.c,v 1.48.6.1 2006/04/22 11:39:59 simonb Exp $");

#include "opt_verified_exec.h"

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/sysctl.h>
#include <sys/inttypes.h>
#define VERIEXEC_NEED_NODE
#include <sys/verified_exec.h>
#if defined(__FreeBSD__)
# include <sys/systm.h>
# include <sys/imgact.h>
# include <crypto/sha1.h>
#else
# include <sys/sha1.h>
#endif
#include <crypto/sha2/sha2.h>
#include <crypto/ripemd160/rmd160.h>
#include <sys/md5.h>
#include <uvm/uvm_extern.h>

int veriexec_verbose;
int veriexec_strict;

char *veriexec_fp_names;
size_t veriexec_name_max;

const struct sysctlnode *veriexec_count_node;

/* Veriexecs table of hash types and their associated information. */
LIST_HEAD(veriexec_ops_head, veriexec_fp_ops) veriexec_ops_list;

/*
 * Add fingerprint names to the global list.
 */
static void
veriexec_add_fp_name(char *name)
{
	char *newp;
	unsigned int new_max;

	if (name == NULL)
		return;

	if (veriexec_fp_names == NULL) {
		veriexec_name_max = (VERIEXEC_TYPE_MAXLEN + 1) * 6;
		veriexec_fp_names = malloc(veriexec_name_max, M_TEMP, M_WAITOK);
		memset(veriexec_fp_names, 0, veriexec_name_max);
	}

	if ((strlen(veriexec_fp_names) + VERIEXEC_TYPE_MAXLEN + 1) >=
	    veriexec_name_max) {
		new_max = veriexec_name_max + 4 * (VERIEXEC_TYPE_MAXLEN + 1);
		newp = realloc(veriexec_fp_names, new_max, M_TEMP, M_WAITOK);
		veriexec_fp_names = newp;
		veriexec_name_max = new_max;
	}

	strlcat(veriexec_fp_names, name, veriexec_name_max);
	strlcat(veriexec_fp_names, " ", veriexec_name_max);
}

/*
 * Add ops to the fignerprint ops vector list.
 */
int veriexec_add_fp_ops(struct veriexec_fp_ops *ops)
{
	if (ops == NULL)
		return (EFAULT);

	if ((ops->init == NULL) ||
	    (ops->update == NULL) ||
	    (ops->final == NULL))
		return (EFAULT);

	ops->type[sizeof(ops->type) - 1] = '\0';

#ifdef DIAGNOSTIC
	if (veriexec_find_ops(ops->type) != NULL)
		return (EEXIST);
#endif /* DIAGNOSTIC */

	LIST_INSERT_HEAD(&veriexec_ops_list, ops, entries);
	veriexec_add_fp_name(ops->type);

	return (0);
}

/*
 * Initialise the internal "default" fingerprint ops vector list.
 */
void
veriexec_init_fp_ops(void)
{
	struct veriexec_fp_ops *ops;

	LIST_INIT(&veriexec_ops_list);
	veriexec_fp_names = NULL;
	veriexec_name_max = 0;

#ifdef VERIFIED_EXEC_FP_RMD160
	ops = (struct veriexec_fp_ops *) malloc(sizeof(*ops), M_TEMP, M_WAITOK);
	VERIEXEC_OPINIT(ops, "RMD160", RMD160_DIGEST_LENGTH,
			sizeof(RMD160_CTX), RMD160Init, RMD160Update,
			RMD160Final);
	(void) veriexec_add_fp_ops(ops);
#endif /* VERIFIED_EXEC_FP_RMD160 */

#ifdef VERIFIED_EXEC_FP_SHA256
	ops = (struct veriexec_fp_ops *) malloc(sizeof(*ops), M_TEMP, M_WAITOK);
	VERIEXEC_OPINIT(ops, "SHA256", SHA256_DIGEST_LENGTH,
			sizeof(SHA256_CTX), SHA256_Init, SHA256_Update,
			SHA256_Final);
	(void) veriexec_add_fp_ops(ops);
#endif /* VERIFIED_EXEC_FP_SHA256 */

#ifdef VERIFIED_EXEC_FP_SHA384
	ops = (struct veriexec_fp_ops *) malloc(sizeof(*ops), M_TEMP, M_WAITOK);
	VERIEXEC_OPINIT(ops, "SHA384", SHA384_DIGEST_LENGTH,
			sizeof(SHA384_CTX), SHA384_Init, SHA384_Update,
			SHA384_Final);
	(void) veriexec_add_fp_ops(ops);
#endif /* VERIFIED_EXEC_FP_SHA384 */

#ifdef VERIFIED_EXEC_FP_SHA512
	ops = (struct veriexec_fp_ops *) malloc(sizeof(*ops), M_TEMP, M_WAITOK);
	VERIEXEC_OPINIT(ops, "SHA512", SHA512_DIGEST_LENGTH,
			sizeof(SHA512_CTX), SHA512_Init, SHA512_Update,
			SHA512_Final);
	(void) veriexec_add_fp_ops(ops);
#endif /* VERIFIED_EXEC_FP_SHA512 */

#ifdef VERIFIED_EXEC_FP_SHA1
	ops = (struct veriexec_fp_ops *) malloc(sizeof(*ops), M_TEMP, M_WAITOK);
	VERIEXEC_OPINIT(ops, "SHA1", SHA1_DIGEST_LENGTH,
			sizeof(SHA1_CTX), SHA1Init, SHA1Update,
			SHA1Final);
	(void) veriexec_add_fp_ops(ops);
#endif /* VERIFIED_EXEC_FP_SHA1 */

#ifdef VERIFIED_EXEC_FP_MD5
	ops = (struct veriexec_fp_ops *) malloc(sizeof(*ops), M_TEMP, M_WAITOK);
	VERIEXEC_OPINIT(ops, "MD5", MD5_DIGEST_LENGTH, sizeof(MD5_CTX),
			MD5Init, MD5Update, MD5Final);
	(void) veriexec_add_fp_ops(ops);
#endif /* VERIFIED_EXEC_FP_MD5 */
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
veriexec_fp_calc(struct lwp *l, struct vnode *vp,
		 struct veriexec_hash_entry *vhe, uint64_t size, u_char *fp)
{
	void *ctx, *page_ctx;
	u_char *buf, *page_fp;
	off_t offset, len;
	size_t resid, npages;
	int error, do_perpage, pagen;

	if (vhe->ops == NULL) {
		panic("veriexec: Operations vector is NULL");
	}

#if 0 /* XXX - for now */
	if ((vhe->type & VERIEXEC_UNTRUSTED) &&
	    (vhe->page_fp_status == PAGE_FP_NONE))
		do_perpage = 1;
	else
#endif
		do_perpage = 0;

	ctx = (void *) malloc(vhe->ops->context_size, M_TEMP, M_WAITOK);
	buf = (u_char *) malloc(PAGE_SIZE, M_TEMP, M_WAITOK);

	page_ctx = NULL;
	page_fp = NULL;
	npages = 0;
	if (do_perpage) {
		npages = (size >> PAGE_SHIFT) + 1;
		page_fp = (u_char *) malloc(vhe->ops->hash_len * npages,
						 M_TEMP, M_WAITOK|M_ZERO);
		vhe->page_fp = page_fp;
		page_ctx = (void *) malloc(vhe->ops->context_size, M_TEMP,
					   M_WAITOK);
	}

	(vhe->ops->init)(ctx);

	len = 0;
	error = 0;
	pagen = 0;
	for (offset = 0; offset < size; offset += PAGE_SIZE) {
		len = ((size - offset) < PAGE_SIZE) ? (size - offset)
						    : PAGE_SIZE;

		error = vn_rdwr(UIO_READ, vp, buf, len, offset, 
				UIO_SYSSPACE,
#ifdef __FreeBSD__
				IO_NODELOCKED,
#else
				0,
#endif
				l->l_proc->p_ucred, &resid, NULL);

		if (error) {
			if (do_perpage) {
				free(vhe->page_fp, M_TEMP);
				vhe->page_fp = NULL;
			}

			goto bad;
		}

		(vhe->ops->update)(ctx, buf, (unsigned int) len);

		if (do_perpage) {
			(vhe->ops->init)(page_ctx);
			(vhe->ops->update)(page_ctx, buf, (unsigned int)len);
			(vhe->ops->final)(page_fp, page_ctx);

			if (veriexec_verbose >= 2) {
				int i;

				printf("hash for page %d: ", pagen);
				for (i = 0; i < vhe->ops->hash_len; i++)
					printf("%02x", page_fp[i]);
				printf("\n");
			}

			page_fp += vhe->ops->hash_len;
			pagen++;
		}

		if (len != PAGE_SIZE)
			break;
	}

	(vhe->ops->final)(fp, ctx);

	if (do_perpage) {
		vhe->last_page_size = len;
		vhe->page_fp_status = PAGE_FP_READY;
		vhe->npages = npages;
	}

bad:
	if (do_perpage)
		free(page_ctx, M_TEMP);
	free(ctx, M_TEMP);
	free(buf, M_TEMP);

	return (error);
}
	
/* Compare two fingerprints of the same type. */
int
veriexec_fp_cmp(struct veriexec_fp_ops *ops, u_char *fp1, u_char *fp2)
{
	if (veriexec_verbose >= 2) {
		int i;

		printf("comparing hashes...\n");
		printf("fp1: ");
		for (i = 0; i < ops->hash_len; i++) {
			printf("%02x", fp1[i]);
		}
		printf("\nfp2: ");
		for (i = 0; i < ops->hash_len; i++) {
			printf("%02x", fp2[i]);
		}
		printf("\n");
	}

	return (memcmp(fp1, fp2, ops->hash_len));
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
		panic("veriexec: veriexec_hashadd: vhh is NULL.");

	LIST_INSERT_HEAD(vhh, e, entries);

	tbl->hash_count++;

	return (0);
}

/*
 * Verify the fingerprint of the given file. If we're called directly from
 * sys_execve(), 'flag' will be VERIEXEC_DIRECT. If we're called from
 * exec_script(), 'flag' will be VERIEXEC_INDIRECT.  If we are called from
 * vn_open(), 'flag' will be VERIEXEC_FILE.
 */
int
veriexec_verify(struct lwp *l, struct vnode *vp, struct vattr *va,
		const u_char *name, int flag, struct veriexec_hash_entry **ret)
{
	struct veriexec_hash_entry *vhe;
	u_char *digest;
	int error;

	if (vp->v_type != VREG)
		return (0);

	/* Lookup veriexec table entry, save pointer if requested. */
	vhe = veriexec_lookup((dev_t)va->va_fsid, (ino_t)va->va_fileid);
	if (ret != NULL)
		*ret = vhe;
	if (vhe == NULL)
		goto out;

	/* Evaluate fingerprint if needed. */
	error = 0;
	digest = NULL;
	if ((vhe->status == FINGERPRINT_NOTEVAL) ||
	    (vhe->type & VERIEXEC_UNTRUSTED)) {
		/* Calculate fingerprint for on-disk file. */
		digest = (u_char *) malloc(vhe->ops->hash_len, M_TEMP,
					   M_WAITOK);
		error = veriexec_fp_calc(l, vp, vhe, va->va_size, digest);
		if (error) {
			veriexec_report("Fingerprint calculation error.",
					name, va, NULL, REPORT_NOVERBOSE,
					REPORT_NOALARM, REPORT_NOPANIC);
			free(digest, M_TEMP);
			return (error);
		}

		/* Compare fingerprint with loaded data. */
		if (veriexec_fp_cmp(vhe->ops, vhe->fp, digest) == 0) {
			vhe->status = FINGERPRINT_VALID;
		} else {
			vhe->status = FINGERPRINT_NOMATCH;
		}

		free(digest, M_TEMP);
	}

	if (!(vhe->type & flag)) {
		veriexec_report("Incorrect access type.", name, va, l,
				REPORT_NOVERBOSE, REPORT_ALARM,
				REPORT_NOPANIC);

		/* IPS mode: Enforce access type. */
		if (veriexec_strict >= 2)
			return (EPERM);
	}

out:
	/* No entry in the veriexec tables. */
	if (vhe == NULL) {
		veriexec_report("veriexec_verify: No entry.", name, va,
		    l, REPORT_VERBOSE, REPORT_NOALARM, REPORT_NOPANIC);

		/* Lockdown mode: Deny access to non-monitored files. */
		if (veriexec_strict >= 3)
			return (EPERM);

		return (0);
	}

        switch (vhe->status) {
	case FINGERPRINT_NOTEVAL:
		/* Should not happen. */
		veriexec_report("veriexec_verify: Not-evaluated status "
		    "post evaluation; inconsistency detected.", name, va,
		    NULL, REPORT_NOVERBOSE, REPORT_NOALARM, REPORT_PANIC);

	case FINGERPRINT_VALID:
		/* Valid fingerprint. */
		veriexec_report("veriexec_verify: Match.", name, va, NULL,
		    REPORT_VERBOSE, REPORT_NOALARM, REPORT_NOPANIC);

		break;

	case FINGERPRINT_NOMATCH:
		/* Fingerprint mismatch. */
		veriexec_report("veriexec_verify: Mismatch.", name, va,
		    NULL, REPORT_NOVERBOSE, REPORT_ALARM, REPORT_NOPANIC);

		/* IDS mode: Deny access on fingerprint mismatch. */
		if (veriexec_strict >= 1)
			error = EPERM;

		break;

	default:
		/*
		 * Should never happen.
		 * XXX: Print vnode/process?
		 */
		veriexec_report("veriexec_verify: Invalid status "
		    "post evaluation.", name, va, NULL, REPORT_NOVERBOSE,
		    REPORT_NOALARM, REPORT_PANIC);
        }

	return (error);
}

/*
 * Evaluate per-page fingerprints.
 */
int
veriexec_page_verify(struct veriexec_hash_entry *vhe, struct vattr *va,
		     struct vm_page *pg, size_t idx, struct lwp *l)
{
	void *ctx;
	u_char *fp;
	u_char *page_fp;
	int error;
	vaddr_t kva;

	if (vhe->page_fp_status == PAGE_FP_NONE)
		return (0);

	if (vhe->page_fp_status == PAGE_FP_FAIL)
		return (EPERM);

	if (idx >= vhe->npages)
		return (0);

	ctx = malloc(vhe->ops->context_size, M_TEMP, M_WAITOK);
	fp = malloc(vhe->ops->hash_len, M_TEMP, M_WAITOK);
	kva = uvm_km_alloc(kernel_map, PAGE_SIZE, 0, UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	pmap_kenter_pa(kva, VM_PAGE_TO_PHYS(pg), VM_PROT_READ);

	page_fp = (u_char *) vhe->page_fp + (vhe->ops->hash_len * idx);
	(vhe->ops->init)(ctx);
	(vhe->ops->update)(ctx, (void *) kva,
			   ((vhe->npages - 1) == idx) ? vhe->last_page_size
						      : PAGE_SIZE);
	(vhe->ops->final)(fp, ctx);

	pmap_kremove(kva, PAGE_SIZE);
	uvm_km_free(kernel_map, kva, PAGE_SIZE, UVM_KMF_VAONLY);

	error = veriexec_fp_cmp(vhe->ops, page_fp, fp);
	if (error) {
		const char *msg;

		if (veriexec_strict > 0) {
			msg = "Pages modified: Killing process.";
		} else {
			msg = "Pages modified.";
			error = 0;
		}

		veriexec_report(msg, "[page_in]", va, l, REPORT_NOVERBOSE,
				REPORT_ALARM, REPORT_NOPANIC);

		if (error) {
			ksiginfo_t ksi;

			KSI_INIT(&ksi);
			ksi.ksi_signo = SIGKILL;
			ksi.ksi_code = SI_NOINFO;
			ksi.ksi_pid = l->l_proc->p_pid;
			ksi.ksi_uid = 0;

			kpsignal(l->l_proc, &ksi, NULL);
		}
	}

	free(ctx, M_TEMP);
	free(fp, M_TEMP);

	return (error);
}

/*
 * Veriexec remove policy code.
 */
int
veriexec_removechk(struct lwp *l, struct vnode *vp, const char *pathbuf)
{
	struct veriexec_hashtbl *tbl;
	struct veriexec_hash_entry *vhe;
	struct vattr va;
	int error;

	error = VOP_GETATTR(vp, &va, l->l_proc->p_ucred, l);
	if (error)
		return (error);

	vhe = veriexec_lookup(va.va_fsid, va.va_fileid);
	if (vhe == NULL) {
		/* Lockdown mode: Deny access to non-monitored files. */
		if (veriexec_strict >= 3)
			return (EPERM);

		return (0);
	}

	veriexec_report("Remove request.", pathbuf, &va, l,
			REPORT_NOVERBOSE, REPORT_ALARM, REPORT_NOPANIC);

	/* IPS mode: Deny removal of monitored files. */
	if (veriexec_strict >= 2)
		return (EPERM);

	tbl = veriexec_tblfind(va.va_fsid);
	if (tbl == NULL) {
		veriexec_report("veriexec_removechk: Inconsistency "
		    "detected: Could not get table for file in lists.",
		    pathbuf, &va, NULL, REPORT_NOVERBOSE, REPORT_NOALARM,
		    REPORT_PANIC);
	}

	LIST_REMOVE(vhe, entries);
	if (vhe->fp != NULL)
		free(vhe->fp, M_TEMP);
	if (vhe->page_fp != NULL)
		free(vhe->page_fp, M_TEMP);
	free(vhe, M_TEMP);
	tbl->hash_count--;

	return (error);
}

/*
 * Veriexe rename policy.
 */
int
veriexec_renamechk(struct vnode *vp, const char *from, const char *to,
		   struct lwp *l)
{
	struct veriexec_hash_entry *vhe;
	struct vattr va;
	int error;

	error = VOP_GETATTR(vp, &va, l->l_proc->p_ucred, l);
	if (error)
		return (error);

	if (veriexec_strict >= 3) {
		printf("Veriexec: veriexec_renamechk: Preventing rename "
		       "of \"%s\" [%ld:%llu] to \"%s\", uid=%u, pid=%u: "
		       "Lockdown mode.\n", from, va.va_fsid,
		       (unsigned long long)va.va_fileid,
		       to, l->l_proc->p_ucred->cr_uid, l->l_proc->p_pid);
		return (EPERM);
	}

	vhe = veriexec_lookup(va.va_fsid, va.va_fileid);
	if (vhe != NULL) {
		if (veriexec_strict >= 2) {
			printf("Veriexec: veriexec_renamechk: Preventing "
			       "rename of \"%s\" [%ld:%llu] to \"%s\", "
			       "uid=%u, pid=%u: IPS mode, file "
			       "monitored.\n", from, va.va_fsid,
			       (unsigned long long)va.va_fileid,
			       to, l->l_proc->p_ucred->cr_uid,
			       l->l_proc->p_pid);
			return (EPERM);
		}

		printf("Veriexec: veriexec_rename: Monitored file \"%s\" "
		       "[%ld:%llu] renamed to \"%s\", uid=%u, pid=%u.\n",
		       from, va.va_fsid, (unsigned long long)va.va_fileid, to,
		       l->l_proc->p_ucred->cr_uid, l->l_proc->p_pid);
	}

	return (0);
}

/*
 * Routine for maintaining mostly consistent message formats in Verified
 * Exec.
 *
 * 'verbose_only' - if 1, the message will be printed only if veriexec is
 * in verbose mode.
 * 'alarm' - if 1, the message is considered an alarm and will be printed
 * at all times along with pid and user credentials.
 * 'die' - if 1, the system will call panic() instead of printf().
 */
void
veriexec_report(const u_char *msg, const u_char *filename,
		struct vattr *va, struct lwp *l, int verbose, int alarm,
		int die)
{
	void (*f)(const char *, ...);

	if (msg == NULL || filename == NULL || va == NULL)
		return;

	if (die)
		f = panic;
	else
		f = (void (*)(const char *, ...)) printf;

	if (!verbose || (verbose <= veriexec_verbose)) {
		if (!alarm || l == NULL)
			f("veriexec: %s [%s, %ld:%" PRIu64 "%s", msg, filename,
			    va->va_fsid, va->va_fileid,
			    die ? "]" : "]\n");
		else
			f("veriexec: %s [%s, %ld:%" PRIu64 ", pid=%u, uid=%u, "
			    "gid=%u%s", msg, filename, va->va_fsid,
			    va->va_fileid, l->l_proc->p_pid,
			    l->l_proc->p_cred->p_ruid,
			    l->l_proc->p_cred->p_rgid, die ? "]" : "]\n");
	}
}
