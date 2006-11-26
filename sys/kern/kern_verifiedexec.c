/*	$NetBSD: kern_verifiedexec.c,v 1.71 2006/11/26 16:22:36 elad Exp $	*/

/*-
 * Copyright 2005 Elad Efrat <elad@NetBSD.org>
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
__KERNEL_RCSID(0, "$NetBSD: kern_verifiedexec.c,v 1.71 2006/11/26 16:22:36 elad Exp $");

#include "opt_veriexec.h"

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
# include <crypto/sha2/sha2.h>
# include <crypto/ripemd160/rmd160.h>
#else
# include <sys/sha1.h>
# include <sys/sha2.h>
# include <sys/rmd160.h>
#endif
#include <sys/md5.h>
#include <uvm/uvm_extern.h>
#include <sys/fileassoc.h>
#include <sys/kauth.h>

int veriexec_verbose;
int veriexec_strict;

char *veriexec_fp_names;
size_t veriexec_name_max;

const struct sysctlnode *veriexec_count_node;

int veriexec_hook;

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

	/*
	 * If we don't have space for any names, allocate enough for six
	 * which should be sufficient. (it's also enough for all algorithms
	 * we can support at the moment)
	 */
	if (veriexec_fp_names == NULL) {
		veriexec_name_max = (VERIEXEC_TYPE_MAXLEN + 1) * 6;
		veriexec_fp_names = malloc(veriexec_name_max, M_TEMP,
		    M_WAITOK|M_ZERO);
	}

	/*
	 * If we're running out of space for storing supported algorithms,
	 * extend the buffer with space for four names.
	 */
	if ((veriexec_name_max - strlen(veriexec_fp_names)) <=
	     VERIEXEC_TYPE_MAXLEN + 1) {
		/* Add space for four algorithm names. */
		new_max = veriexec_name_max + 4 * (VERIEXEC_TYPE_MAXLEN + 1);
		newp = realloc(veriexec_fp_names, new_max, M_TEMP, M_WAITOK);
		veriexec_fp_names = newp;
		veriexec_name_max = new_max;
	}

	if (*veriexec_fp_names != '\0')
		strlcat(veriexec_fp_names, " ", veriexec_name_max);

	strlcat(veriexec_fp_names, name, veriexec_name_max);
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

	if (veriexec_find_ops(ops->type) != NULL)
		return (EEXIST);

	LIST_INSERT_HEAD(&veriexec_ops_list, ops, entries);
	veriexec_add_fp_name(ops->type);

	return (0);
}

/*
 * Initialise Veriexec.
 */
void
veriexec_init(void)
{
	struct veriexec_fp_ops *ops;

	/* Register a fileassoc for Veriexec. */
	veriexec_hook = fileassoc_register("veriexec", veriexec_clear);
	if (veriexec_hook == FILEASSOC_INVAL)
		panic("Veriexec: Can't register fileassoc");

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

	if ((name == NULL) || (strlen(name) == 0))
		return (NULL);

	name[VERIEXEC_TYPE_MAXLEN - 1] = '\0';

	LIST_FOREACH(ops, &veriexec_ops_list, entries) {
		if (strncasecmp(name, ops->type, sizeof(ops->type) - 1) == 0)
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
    struct veriexec_file_entry *vfe, u_char *fp)
{
	struct vattr va;
	void *ctx, *page_ctx;
	u_char *buf, *page_fp;
	off_t offset, len;
	size_t resid, npages;
	int error, do_perpage, pagen;

	error = VOP_GETATTR(vp, &va, l->l_cred, l);
	if (error)
		return (error);

#if 0 /* XXX - for now */
	if ((vfe->type & VERIEXEC_UNTRUSTED) &&
	    (vfe->page_fp_status == PAGE_FP_NONE))
		do_perpage = 1;
	else
#endif
		do_perpage = 0;

	ctx = (void *) malloc(vfe->ops->context_size, M_TEMP, M_WAITOK);
	buf = (u_char *) malloc(PAGE_SIZE, M_TEMP, M_WAITOK);

	page_ctx = NULL;
	page_fp = NULL;
	npages = 0;
	if (do_perpage) {
		npages = (va.va_size >> PAGE_SHIFT) + 1;
		page_fp = (u_char *) malloc(vfe->ops->hash_len * npages,
						 M_TEMP, M_WAITOK|M_ZERO);
		vfe->page_fp = page_fp;
		page_ctx = (void *) malloc(vfe->ops->context_size, M_TEMP,
					   M_WAITOK);
	}

	(vfe->ops->init)(ctx);

	len = 0;
	error = 0;
	pagen = 0;
	for (offset = 0; offset < va.va_size; offset += PAGE_SIZE) {
		len = ((va.va_size - offset) < PAGE_SIZE) ?
		    (va.va_size - offset) : PAGE_SIZE;

		error = vn_rdwr(UIO_READ, vp, buf, len, offset,
				UIO_SYSSPACE,
#ifdef __FreeBSD__
				IO_NODELOCKED,
#else
				0,
#endif
				l->l_cred, &resid, NULL);

		if (error) {
			if (do_perpage) {
				free(vfe->page_fp, M_TEMP);
				vfe->page_fp = NULL;
			}

			goto bad;
		}

		(vfe->ops->update)(ctx, buf, (unsigned int) len);

		if (do_perpage) {
			(vfe->ops->init)(page_ctx);
			(vfe->ops->update)(page_ctx, buf, (unsigned int)len);
			(vfe->ops->final)(page_fp, page_ctx);

			if (veriexec_verbose >= 2) {
				int i;

				printf("hash for page %d: ", pagen);
				for (i = 0; i < vfe->ops->hash_len; i++)
					printf("%02x", page_fp[i]);
				printf("\n");
			}

			page_fp += vfe->ops->hash_len;
			pagen++;
		}

		if (len != PAGE_SIZE)
			break;
	}

	(vfe->ops->final)(fp, ctx);

	if (do_perpage) {
		vfe->last_page_size = len;
		vfe->page_fp_status = PAGE_FP_READY;
		vfe->npages = npages;
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

struct veriexec_table_entry *
veriexec_tblfind(struct vnode *vp)
{
	return (fileassoc_tabledata_lookup(vp->v_mount, veriexec_hook));
}

struct veriexec_file_entry *
veriexec_lookup(struct vnode *vp)
{
	return (fileassoc_lookup(vp, veriexec_hook));
}

/*
 * Add an entry to a hash table. If a collision is found, handle it.
 * The passed entry is allocated in kernel memory.
 */
int
veriexec_hashadd(struct vnode *vp, struct veriexec_file_entry *vfe)
{
	struct veriexec_table_entry *vte;
	int error;

	error = fileassoc_add(vp, veriexec_hook, vfe);
	if (error)
		return (error);

	vte = veriexec_tblfind(vp);
	KASSERT(vte != NULL);

	vte->vte_count++;

	return (0);
}

/*
 * Verify the fingerprint of the given file. If we're called directly from
 * sys_execve(), 'flag' will be VERIEXEC_DIRECT. If we're called from
 * exec_script(), 'flag' will be VERIEXEC_INDIRECT.  If we are called from
 * vn_open(), 'flag' will be VERIEXEC_FILE.
 */
int
veriexec_verify(struct lwp *l, struct vnode *vp, const u_char *name, int flag,
    struct veriexec_file_entry **ret)
{
	struct veriexec_file_entry *vfe;
	u_char *digest;
	int error;

	if (vp->v_type != VREG)
		return (0);

	/* Lookup veriexec table entry, save pointer if requested. */
	vfe = veriexec_lookup(vp);
	if (ret != NULL)
		*ret = vfe;
	if (vfe == NULL)
		goto out;

	/* Evaluate fingerprint if needed. */
	error = 0;
	digest = NULL;
	if ((vfe->status == FINGERPRINT_NOTEVAL) ||
	    (vfe->type & VERIEXEC_UNTRUSTED)) {
		/* Calculate fingerprint for on-disk file. */
		digest = (u_char *) malloc(vfe->ops->hash_len, M_TEMP,
					   M_WAITOK);
		error = veriexec_fp_calc(l, vp, vfe, digest);
		if (error) {
			veriexec_report("Fingerprint calculation error.",
			    name, NULL, REPORT_ALWAYS);
			free(digest, M_TEMP);
			return (error);
		}

		/* Compare fingerprint with loaded data. */
		if (veriexec_fp_cmp(vfe->ops, vfe->fp, digest) == 0) {
			vfe->status = FINGERPRINT_VALID;
		} else {
			vfe->status = FINGERPRINT_NOMATCH;
		}

		free(digest, M_TEMP);
	}

	if (!(vfe->type & flag)) {
		veriexec_report("Incorrect access type.", name, l,
		    REPORT_ALWAYS|REPORT_ALARM);

		/* IPS mode: Enforce access type. */
		if (veriexec_strict >= VERIEXEC_IPS)
			return (EPERM);
	}

 out:
	/* No entry in the veriexec tables. */
	if (vfe == NULL) {
		veriexec_report("No entry.", name,
		    l, REPORT_VERBOSE);

		/*
		 * Lockdown mode: Deny access to non-monitored files.
		 * IPS mode: Deny execution of non-monitored files.
		 */
		if ((veriexec_strict >= VERIEXEC_LOCKDOWN) ||
		    ((veriexec_strict >= VERIEXEC_IPS) &&
		     (flag != VERIEXEC_FILE)))
			return (EPERM);

		return (0);
	}

        switch (vfe->status) {
	case FINGERPRINT_NOTEVAL:
		/* Should not happen. */
		veriexec_report("Not-evaluated status "
		    "post evaluation; inconsistency detected.", name,
		    NULL, REPORT_ALWAYS|REPORT_PANIC);

	case FINGERPRINT_VALID:
		/* Valid fingerprint. */
		veriexec_report("Match.", name, NULL,
		    REPORT_VERBOSE);

		break;

	case FINGERPRINT_NOMATCH:
		/* Fingerprint mismatch. */
		veriexec_report("Mismatch.", name,
		    NULL, REPORT_ALWAYS|REPORT_ALARM);

		/* IDS mode: Deny access on fingerprint mismatch. */
		if (veriexec_strict >= VERIEXEC_IDS)
			error = EPERM;

		break;

	default:
		/* Should never happen. */
		veriexec_report("Invalid status "
		    "post evaluation.", name, NULL, REPORT_ALWAYS|REPORT_PANIC);
        }

	return (error);
}

/*
 * Evaluate per-page fingerprints.
 */
int
veriexec_page_verify(struct veriexec_file_entry *vfe, struct vm_page *pg,
    size_t idx, struct lwp *l)
{
	void *ctx;
	u_char *fp;
	u_char *page_fp;
	int error;
	vaddr_t kva;

	if (vfe->page_fp_status == PAGE_FP_NONE)
		return (0);

	if (vfe->page_fp_status == PAGE_FP_FAIL)
		return (EPERM);

	if (idx >= vfe->npages)
		return (0);

	ctx = malloc(vfe->ops->context_size, M_TEMP, M_WAITOK);
	fp = malloc(vfe->ops->hash_len, M_TEMP, M_WAITOK);
	kva = uvm_km_alloc(kernel_map, PAGE_SIZE, 0, UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	pmap_kenter_pa(kva, VM_PAGE_TO_PHYS(pg), VM_PROT_READ);

	page_fp = (u_char *) vfe->page_fp + (vfe->ops->hash_len * idx);
	(vfe->ops->init)(ctx);
	(vfe->ops->update)(ctx, (void *) kva,
			   ((vfe->npages - 1) == idx) ? vfe->last_page_size
						      : PAGE_SIZE);
	(vfe->ops->final)(fp, ctx);

	pmap_kremove(kva, PAGE_SIZE);
	uvm_km_free(kernel_map, kva, PAGE_SIZE, UVM_KMF_VAONLY);

	error = veriexec_fp_cmp(vfe->ops, page_fp, fp);
	if (error) {
		const char *msg;

		if (veriexec_strict > VERIEXEC_LEARNING) {
			msg = "Pages modified: Killing process.";
		} else {
			msg = "Pages modified.";
			error = 0;
		}

		veriexec_report(msg, "[page_in]", l, REPORT_ALWAYS|REPORT_ALARM);

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
veriexec_removechk(struct vnode *vp, const char *pathbuf, struct lwp *l)
{
	struct veriexec_file_entry *vfe;
	struct veriexec_table_entry *vte;

	vfe = veriexec_lookup(vp);
	if (vfe == NULL) {
		/* Lockdown mode: Deny access to non-monitored files. */
		if (veriexec_strict >= VERIEXEC_LOCKDOWN)
			return (EPERM);

		return (0);
	}

	veriexec_report("Remove request.", pathbuf, l, REPORT_ALWAYS|REPORT_ALARM);

	/* IDS mode: Deny removal of monitored files. */
	if (veriexec_strict >= VERIEXEC_IDS)
		return (EPERM);

	fileassoc_clear(vp, veriexec_hook);

	vte = veriexec_tblfind(vp);
	KASSERT(vte != NULL);

	vte->vte_count--;

	return (0);
}

/*
 * Veriexe rename policy.
 */
int
veriexec_renamechk(struct vnode *fromvp, const char *fromname,
    struct vnode *tovp, const char *toname, struct lwp *l)
{
	struct veriexec_file_entry *vfe, *tvfe;

	if (veriexec_strict >= VERIEXEC_LOCKDOWN) {
		log(LOG_ALERT, "Veriexec: Preventing rename of `%s' to "
		    "`%s', uid=%u, pid=%u: Lockdown mode.\n", fromname, toname,
		    kauth_cred_geteuid(l->l_cred), l->l_proc->p_pid);
		return (EPERM);
	}

	vfe = veriexec_lookup(fromvp);
	tvfe = NULL;
	if (tovp != NULL)
		tvfe = veriexec_lookup(tovp);

	if ((vfe != NULL) || (tvfe != NULL)) {
		if (veriexec_strict >= VERIEXEC_IPS) {
			log(LOG_ALERT, "Veriexec: Preventing rename of `%s' "
			    "to `%s', uid=%u, pid=%u: IPS mode, file "
			    "monitored.\n", fromname, toname,
			    kauth_cred_geteuid(l->l_cred),
			    l->l_proc->p_pid);
			return (EPERM);
		}

		log(LOG_NOTICE, "Veriexec: Monitored file `%s' renamed to "
		    "`%s', uid=%u, pid=%u.\n", fromname, toname,
		    kauth_cred_geteuid(l->l_cred), l->l_proc->p_pid);
	}

	return (0);
}

/*
 * Routine for maintaining mostly consistent message formats in Verified
 * Exec.
 */
void
veriexec_report(const u_char *msg, const u_char *filename, struct lwp *l, int f)
{
	if (msg == NULL || filename == NULL)
		return;

	if (((f & REPORT_LOGMASK) >> 1) <= veriexec_verbose) {
		if (!(f & REPORT_ALARM) || (l == NULL))
			log(LOG_NOTICE, "Veriexec: %s [%s]\n", msg,
			    filename);
		else
			log(LOG_ALERT, "Veriexec: %s [%s, pid=%u, uid=%u, "
			    "gid=%u]\n", msg, filename, l->l_proc->p_pid,
			    kauth_cred_getuid(l->l_cred),
			    kauth_cred_getgid(l->l_cred));
	}

	if (f & REPORT_PANIC)
		panic("Veriexec: Unrecoverable error.");
}

void
veriexec_clear(void *data, int file_specific)
{
	if (file_specific) {
		struct veriexec_file_entry *vfe = data;

		if (vfe != NULL) {
			if (vfe->fp != NULL)
				free(vfe->fp, M_TEMP);
			if (vfe->page_fp != NULL)
				free(vfe->page_fp, M_TEMP);
			free(vfe, M_TEMP);
		}
	} else {
		struct veriexec_table_entry *vte = data;

		if (vte != NULL)
			free(vte, M_TEMP);
	}
}

/*
 * Invalidate a Veriexec file entry.
 * XXX: This should be updated when per-page fingerprints are added.
 */
void
veriexec_purge(struct veriexec_file_entry *vfe)
{
	vfe->status = FINGERPRINT_NOTEVAL;
}

/*
 * Enforce raw disk access policy.
 *
 * IDS mode: Invalidate fingerprints on a mount if it's opened for writing.
 * IPS mode: Don't allow raw writing to disks we monitor.
 * Lockdown mode: Don't allow raw writing to all disks.
 *
 * XXX: This is bogus. There's an obvious race condition between the time
 * XXX: the disk is open for writing, in which an attacker can access a
 * XXX: monitored file to get its signature cached again, and when the raw
 * XXX: file is overwritten on disk.
 * XXX:
 * XXX: To solve this, we need something like the following:
 * XXX:		open raw disk:
 * XXX:		  - raise refcount,
 * XXX:		  - invalidate fingerprints,
 * XXX:		  - mark all entries for that disk with "no cache" flag
 * XXX:
 * XXX:		veriexec_verify:
 * XXX:		  - if "no cache", don't cache evaluation result
 * XXX:
 * XXX:		close raw disk:
 * XXX:		  - lower refcount,
 * XXX:		  - if refcount == 0, remove "no cache" flag from all entries
 */
int
veriexec_rawchk(struct vnode *vp)
{
	int monitored;

	monitored = (vp && veriexec_tblfind(vp));

	switch (veriexec_strict) {
	case VERIEXEC_IDS:
		if (monitored)
			fileassoc_table_run(vp->v_mount, veriexec_hook,
			    (fileassoc_cb_t)veriexec_purge);
		break;
	case VERIEXEC_IPS:
		if (monitored)
			return (EPERM);
		break;
	case VERIEXEC_LOCKDOWN:
		return (EPERM);
	}

	return (0);
}
