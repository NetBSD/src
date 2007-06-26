/*	$NetBSD: verified_exec.h,v 1.6.2.11 2007/06/26 15:23:59 ghen Exp $	*/

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

/*
 *
 * Definitions for the Verified Executables kernel function.
 *
 */
#ifndef _SYS_VERIFIED_EXEC_H_
#define _SYS_VERIFIED_EXEC_H_

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/hash.h>

__KERNEL_RCSID(0, "$NetBSD: verified_exec.h,v 1.6.2.11 2007/06/26 15:23:59 ghen Exp $");

/* Max length of the fingerprint type string, including terminating \0 char */
#define VERIEXEC_TYPE_MAXLEN 9

struct veriexec_params  {
	unsigned char type;
	unsigned char fp_type[VERIEXEC_TYPE_MAXLEN];
	char file[MAXPATHLEN];
	unsigned int size;  /* number of bytes in the fingerprint */
	unsigned char *fingerprint;
};

struct veriexec_sizing_params {
	dev_t dev;
	size_t hash_size;
};

/*
 * Types of veriexec inodes we can have. Ordered from less strict to
 * most strict -- this is enforced if a duplicate entry is loaded.
 */
#define VERIEXEC_DIRECT		0x01 /* Direct execution (exec) */
#define VERIEXEC_INDIRECT	0x02 /* Indirect execution (#!) */
#define VERIEXEC_FILE		0x04 /* Plain file (open) */

#define VERIEXEC_LOAD _IOW('S', 0x1, struct veriexec_params)
#define VERIEXEC_TABLESIZE _IOW('S', 0x2, struct veriexec_sizing_params)

/* Verified exec sysctl objects. */
#define	VERIEXEC_VERBOSE	1 /* Verbosity level. */
#define	VERIEXEC_STRICT		2 /* Strict mode level. */
#define	VERIEXEC_ALGORITHMS	3 /* Supported hashing algorithms. */
#define	VERIEXEC_COUNT		4 /* # of fingerprinted files on device. */

#ifdef _KERNEL
void	veriexecattach(struct device *, struct device *, void *);
int     veriexecopen(dev_t, int, int, struct proc *);
int     veriexecclose(dev_t, int, int, struct proc *);
int     veriexecioctl(dev_t, u_long, caddr_t, int, struct proc *);

/* defined in kern_verifiedexec.c */
extern char *veriexec_fp_names;
extern int veriexec_verbose;
extern int veriexec_strict;
/* this one requires sysctl.h to be included before verified_exec.h */
#ifdef VERIEXEC_NEED_NODE
extern struct sysctlnode *veriexec_count_node;
#endif /* VERIEXEC_NEED_NODE */

/*
 * Operations vector for verified exec, this defines the characteristics
 * for the fingerprint type.
 */

/* Function types: init, update, final. */
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

/*
 * list structure definitions - needed in kern_exec.c
 */

/* An entry in the per-device hash table. */
struct veriexec_hash_entry {
        ino_t         inode;                        /* Inode number. */
        unsigned char type;                         /* Entry type. */
	unsigned char status;			    /* Evaluation status. */
        unsigned char *fp;                          /* Fingerprint. */
	struct veriexec_fp_ops *ops;                /* Fingerprint ops vector*/
        LIST_ENTRY(veriexec_hash_entry) entries;    /* List pointer. */
};

/* Valid status field values. */
#define FINGERPRINT_NOTEVAL  0  /* fingerprint has not been evaluated */
#define FINGERPRINT_VALID    1  /* fingerprint evaluated and matches list */
#define FINGERPRINT_NOMATCH  2  /* fingerprint evaluated but does not match */

LIST_HEAD(veriexec_hashhead, veriexec_hash_entry) *hash_tbl;

/* Veriexec hash table information. */
struct veriexec_hashtbl {
        struct veriexec_hashhead *hash_tbl;
        size_t hash_size;       /* Number of slots in the table. */
        dev_t hash_dev;         /* Device ID the hash table refers to. */
	uint64_t hash_count;	/* # of fingerprinted files in table. */
        LIST_ENTRY(veriexec_hashtbl) hash_list;
};

/* Global list of hash tables. */
LIST_HEAD(, veriexec_hashtbl) veriexec_tables;

/* Mask to ensure bounded access to elements in the hash table. */
#define VERIEXEC_HASH_MASK(tbl)    ((tbl)->hash_size - 1)

/* Readable values for veriexec_report(). */
#define	REPORT_NOVERBOSE	0
#define	REPORT_VERBOSE		1
#define	REPORT_VERBOSE_HIGH	2
#define	REPORT_NOPANIC		0
#define	REPORT_PANIC		1
#define	REPORT_NOALARM		0
#define	REPORT_ALARM		1

/*
 * Hashing function: Takes an inode number modulus the mask to give back
 * an index into the hash table.
 */
#define VERIEXEC_HASH(tbl, inode)  \
        (hash32_buf(&(inode), sizeof((inode)), HASH32_BUF_INIT) \
	 & VERIEXEC_HASH_MASK(tbl))

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
void veriexec_init_fp_ops(void);
struct veriexec_fp_ops *veriexec_find_ops(u_char *name);
int veriexec_fp_calc(struct proc *, struct vnode *,
		     struct veriexec_hash_entry *, uint64_t, u_char *);
int veriexec_fp_cmp(struct veriexec_fp_ops *, u_char *, u_char *);
struct veriexec_hashtbl *veriexec_tblfind(dev_t);
struct veriexec_hash_entry *veriexec_lookup(dev_t, ino_t);
int veriexec_hashadd(struct veriexec_hashtbl *, struct veriexec_hash_entry *);
int veriexec_verify(struct proc *, struct vnode *, struct vattr *,
		    const u_char *, int, struct veriexec_hash_entry **);
int veriexec_removechk(struct proc *, struct vnode *, const char *);
int veriexec_renamechk(struct vnode *, const char *, const char *);
void veriexec_init_fp_ops(void);
void veriexec_report(const u_char *, const u_char *, struct vattr *,
		     struct proc *, int, int, int);

#endif /* _KERNEL */

#endif /* _SYS_VERIFIED_EXEC_H_ */
