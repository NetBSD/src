/*	$NetBSD: verified_exec.h,v 1.41 2006/11/26 20:27:27 elad Exp $	*/

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

#ifndef _SYS_VERIFIED_EXEC_H_
#define _SYS_VERIFIED_EXEC_H_

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/hash.h>
#include <uvm/uvm_extern.h>
#include <uvm/uvm_pglist.h>
#include <uvm/uvm_page.h>

/* Max length of the fingerprint type string, including terminating \0 char */
#define	VERIEXEC_TYPE_MAXLEN	9

struct veriexec_params  {
	unsigned char type;
	unsigned char fp_type[VERIEXEC_TYPE_MAXLEN];
	char file[MAXPATHLEN];
	unsigned int size;  /* number of bytes in the fingerprint */
	unsigned char *fingerprint;
};

struct veriexec_sizing_params {
	size_t hash_size;
	u_char file[MAXPATHLEN];
};

struct veriexec_delete_params {
	u_char file[MAXPATHLEN];
};

struct veriexec_query_params {
	u_char file[MAXPATHLEN];
	unsigned char fp_type[VERIEXEC_TYPE_MAXLEN];
	unsigned char type;
	unsigned char status;
	unsigned char *fp;
	size_t fp_bufsize;
	size_t hash_len;
	void *uaddr;
};

/* Flags for a Veriexec entry. These can be OR'd together. */
#define VERIEXEC_DIRECT		0x01 /* Direct execution (exec) */
#define VERIEXEC_INDIRECT	0x02 /* Indirect execution (#!) */
#define VERIEXEC_FILE		0x04 /* Plain file (open) */
#define	VERIEXEC_UNTRUSTED	0x10 /* Untrusted storage */

/* Operations for /dev/veriexec. */
#define VERIEXEC_LOAD		_IOW('S', 0x1, struct veriexec_params)
#define VERIEXEC_TABLESIZE	_IOW('S', 0x2, struct veriexec_sizing_params)
#define VERIEXEC_DELETE		_IOW('S', 0x3, struct veriexec_delete_params)
#define VERIEXEC_QUERY		_IOW('S', 0x4, struct veriexec_query_params)

/* Verified exec sysctl objects. */
#define	VERIEXEC_VERBOSE	1 /* Verbosity level. */
#define	VERIEXEC_STRICT		2 /* Strict mode level. */
#define	VERIEXEC_ALGORITHMS	3 /* Supported hashing algorithms. */
#define	VERIEXEC_COUNT		4 /* # of fingerprinted files on device. */

/* Valid status field values. */
#define FINGERPRINT_NOTEVAL  0  /* fingerprint has not been evaluated */
#define FINGERPRINT_VALID    1  /* fingerprint evaluated and matches list */
#define FINGERPRINT_NOMATCH  2  /* fingerprint evaluated but does not match */

/* Per-page fingerprint status. */
#define	PAGE_FP_NONE	0	/* no per-page fingerprints. */
#define	PAGE_FP_READY	1	/* per-page fingerprints ready for use. */
#define	PAGE_FP_FAIL	2	/* mismatch in per-page fingerprints. */

#ifdef _KERNEL
void	veriexecattach(struct device *, struct device *, void *);
int     veriexecopen(dev_t, int, int, struct lwp *);
int     veriexecclose(dev_t, int, int, struct lwp *);
int     veriexecioctl(dev_t, u_long, caddr_t, int, struct lwp *);

/* defined in kern_verifiedexec.c */
extern char *veriexec_fp_names;
extern int veriexec_verbose;
extern int veriexec_strict;
/* this one requires sysctl.h to be included before verified_exec.h */
#ifdef VERIEXEC_NEED_NODE
extern const struct sysctlnode *veriexec_count_node;
#endif /* VERIEXEC_NEED_NODE */
extern int veriexec_hook;

/*
 * Operations vector for verified exec, this defines the characteristics
 * for the fingerprint type.
 * Function types: init, update, final.
 */
typedef void (*VERIEXEC_INIT_FN)(void *);
typedef void (*VERIEXEC_UPDATE_FN)(void *, u_char *, u_int);
typedef void (*VERIEXEC_FINAL_FN)(u_char *, void *);

struct veriexec_fp_ops {
	char type[VERIEXEC_TYPE_MAXLEN];
	size_t hash_len;
	size_t context_size;
	VERIEXEC_INIT_FN init;
	VERIEXEC_UPDATE_FN update;
	VERIEXEC_FINAL_FN final;
	LIST_ENTRY(veriexec_fp_ops) entries;
};

/* Veriexec per-file entry data. */
struct veriexec_file_entry {
	u_char type;				/* Entry type. */
	u_char status;				/* Evaluation status. */
	u_char page_fp_status;			/* Per-page FP status. */
	u_char *fp;				/* Fingerprint. */
	void *page_fp;				/* Per-page fingerprints */
	size_t npages;			    	/* Number of pages. */
	size_t last_page_size;			/* To support < PAGE_SIZE */
	struct veriexec_fp_ops *ops;		/* Fingerprint ops vector*/
};

/* Veriexec per-table data. */
struct veriexec_table_entry {
	uint64_t vte_count;			/* Number of Veriexec entries. */
	const struct sysctlnode *vte_node;
};

/* Veriexec modes (strict levels). */
#define	VERIEXEC_LEARNING	0	/* Learning mode. */
#define	VERIEXEC_IDS		1	/* Intrusion detection mode. */
#define	VERIEXEC_IPS		2	/* Intrusion prevention mode. */
#define	VERIEXEC_LOCKDOWN	3	/* Lockdown mode. */

/* Readable values for veriexec_report(). */
#define	REPORT_ALWAYS		0x01	/* Always print */
#define	REPORT_VERBOSE		0x02	/* Print when verbose >= 1 */
#define	REPORT_DEBUG		0x04	/* Print when verbose >= 2 (debug) */
#define	REPORT_PANIC		0x08	/* Call panic() */
#define	REPORT_ALARM		0x10	/* Alarm - also print pid/uid/.. */
#define	REPORT_LOGMASK		(REPORT_ALWAYS|REPORT_VERBOSE|REPORT_DEBUG)

/* Initialize a fingerprint ops struct. */
#define	VERIEXEC_OPINIT(ops, fp_type, hashlen, ctx_size, init_fn, \
	update_fn, final_fn) \
	do {								    \
		(void) strlcpy((ops)->type, fp_type, sizeof((ops)->type));  \
		(ops)->hash_len = (hashlen);				    \
		(ops)->context_size = (ctx_size);			    \
		(ops)->init = (VERIEXEC_INIT_FN) (init_fn);		    \
		(ops)->update = (VERIEXEC_UPDATE_FN) (update_fn);	    \
		(ops)->final = (VERIEXEC_FINAL_FN) (final_fn);		    \
	} while (0);

int veriexec_add_fp_ops(struct veriexec_fp_ops *);
void veriexec_init(void);
struct veriexec_fp_ops *veriexec_find_ops(u_char *name);
int veriexec_fp_calc(struct lwp *, struct vnode *, struct veriexec_file_entry *,
    u_char *);
int veriexec_fp_cmp(struct veriexec_fp_ops *, u_char *, u_char *);
struct veriexec_table_entry *veriexec_tblfind(struct vnode *);
struct veriexec_file_entry *veriexec_lookup(struct vnode *);
int veriexec_hashadd(struct vnode *, struct veriexec_file_entry *);
int veriexec_verify(struct lwp *, struct vnode *,
    const u_char *, int, struct veriexec_file_entry **);
int veriexec_page_verify(struct veriexec_file_entry *, struct vm_page *, size_t,
    struct lwp *);
int veriexec_removechk(struct vnode *, const char *, struct lwp *l);
int veriexec_renamechk(struct vnode *, const char *, struct vnode *,
    const char *, struct lwp *);
void veriexec_report(const u_char *, const u_char *, struct lwp *, int);
int veriexec_newtable(struct veriexec_sizing_params *, struct lwp *);
int veriexec_load(struct veriexec_params *, struct lwp *);
int veriexec_delete(struct veriexec_delete_params *, struct lwp *);
int veriexec_query(struct veriexec_query_params *, struct lwp *);
void veriexec_clear(void *, int);
void veriexec_purge(struct veriexec_file_entry *);

#endif /* _KERNEL */

#endif /* !_SYS_VERIFIED_EXEC_H_ */
