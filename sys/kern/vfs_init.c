/*	$NetBSD: vfs_init.c,v 1.16 2000/08/03 20:41:23 thorpej Exp $	*/

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed
 * to Berkeley by John Heidemann of the UCLA Ficus project.
 *
 * Source: * @(#)i405_init.c 2.10 92/04/27 UCLA Ficus project
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vfs_init.c	8.5 (Berkeley) 5/11/95
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/namei.h>
#include <sys/ucred.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/systm.h>

/*
 * Sigh, such primitive tools are these...
 */
#if 0
#define DODEBUG(A) A
#else
#define DODEBUG(A)
#endif

/*
 * The global list of vnode operations.
 */
extern struct vnodeop_desc *vfs_op_descs[];

/*
 * These vnodeopv_descs are listed here because they are not
 * associated with any particular file system, and thus cannot
 * be initialized by vfs_attach().
 */
extern struct vnodeopv_desc dead_vnodeop_opv_desc;
extern struct vnodeopv_desc fifo_vnodeop_opv_desc;
extern struct vnodeopv_desc spec_vnodeop_opv_desc;
extern struct vnodeopv_desc sync_vnodeop_opv_desc;

struct vnodeopv_desc *vfs_special_vnodeopv_descs[] = {
	&dead_vnodeop_opv_desc,
	&fifo_vnodeop_opv_desc,
	&spec_vnodeop_opv_desc,
	&sync_vnodeop_opv_desc,
	NULL,
};

/*
 * This code doesn't work if the defn is **vnodop_defns with cc.
 * The problem is because of the compiler sometimes putting in an
 * extra level of indirection for arrays.  It's an interesting
 * "feature" of C.
 */
int vfs_opv_numops;

typedef int (*PFI) __P((void *));

void vfs_opv_init_explicit __P((struct vnodeopv_desc *));
void vfs_opv_init_default __P((struct vnodeopv_desc *));
void vfs_op_init __P((void));

/*
 * A miscellaneous routine.
 * A generic "default" routine that just returns an error.
 */
/*ARGSUSED*/
int
vn_default_error(v)
	void *v;
{

	return (EOPNOTSUPP);
}

/*
 * vfs_init.c
 *
 * Allocate and fill in operations vectors.
 *
 * An undocumented feature of this approach to defining operations is that
 * there can be multiple entries in vfs_opv_descs for the same operations
 * vector. This allows third parties to extend the set of operations
 * supported by another layer in a binary compatibile way. For example,
 * assume that NFS needed to be modified to support Ficus. NFS has an entry
 * (probably nfs_vnopdeop_decls) declaring all the operations NFS supports by
 * default. Ficus could add another entry (ficus_nfs_vnodeop_decl_entensions)
 * listing those new operations Ficus adds to NFS, all without modifying the
 * NFS code. (Of couse, the OTW NFS protocol still needs to be munged, but
 * that is a(whole)nother story.) This is a feature.
 */

/*
 * Init the vector, if it needs it.
 * Also handle backwards compatibility.
 */
void
vfs_opv_init_explicit(vfs_opv_desc)
	struct vnodeopv_desc *vfs_opv_desc;
{
	int (**opv_desc_vector) __P((void *));
	struct vnodeopv_entry_desc *opve_descp;

	opv_desc_vector = *(vfs_opv_desc->opv_desc_vector_p);

	for (opve_descp = vfs_opv_desc->opv_desc_ops;
	     opve_descp->opve_op;
	     opve_descp++) {
		/*
		 * Sanity check:  is this operation listed
		 * in the list of operations?  We check this
		 * by seeing if its offest is zero.  Since
		 * the default routine should always be listed
		 * first, it should be the only one with a zero
		 * offset.  Any other operation with a zero
		 * offset is probably not listed in
		 * vfs_op_descs, and so is probably an error.
		 *
		 * A panic here means the layer programmer
		 * has committed the all-too common bug
		 * of adding a new operation to the layer's
		 * list of vnode operations but
		 * not adding the operation to the system-wide
		 * list of supported operations.
		 */
		if (opve_descp->opve_op->vdesc_offset == 0 &&
		    opve_descp->opve_op->vdesc_offset != VOFFSET(vop_default)) {
			printf("operation %s not listed in %s.\n",
			    opve_descp->opve_op->vdesc_name, "vfs_op_descs");
			panic ("vfs_opv_init: bad operation");
		}

		/*
		 * Fill in this entry.
		 */
		opv_desc_vector[opve_descp->opve_op->vdesc_offset] =
		    opve_descp->opve_impl;
	}
}

void
vfs_opv_init_default(vfs_opv_desc)
	struct vnodeopv_desc *vfs_opv_desc;
{
	int j;
	int (**opv_desc_vector) __P((void *));

	opv_desc_vector = *(vfs_opv_desc->opv_desc_vector_p);

	/*
	 * Force every operations vector to have a default routine.
	 */
	if (opv_desc_vector[VOFFSET(vop_default)] == NULL)
		panic("vfs_opv_init: operation vector without default routine.");

	for (j = 0; j < vfs_opv_numops; j++)
		if (opv_desc_vector[j] == NULL)
			opv_desc_vector[j] = 
			    opv_desc_vector[VOFFSET(vop_default)];
}

void
vfs_opv_init(vopvdpp)
	struct vnodeopv_desc **vopvdpp;
{
	int (**opv_desc_vector) __P((void *));
	int i;

	/*
	 * Allocate the vectors.
	 */
	for (i = 0; vopvdpp[i] != NULL; i++) {
		/* XXX - shouldn't be M_VNODE */
		opv_desc_vector =
		    malloc(vfs_opv_numops * sizeof(PFI), M_VNODE, M_WAITOK);
		memset(opv_desc_vector, 0, vfs_opv_numops * sizeof(PFI));
		*(vopvdpp[i]->opv_desc_vector_p) = opv_desc_vector;
		DODEBUG(printf("vector at %p allocated\n",
		    opv_desc_vector_p));
	}

	/*
	 * ...and fill them in.
	 */
	for (i = 0; vopvdpp[i] != NULL; i++)
		vfs_opv_init_explicit(vopvdpp[i]);

	/*
	 * Finally, go back and replace unfilled routines
	 * with their default.
	 */
	for (i = 0; vopvdpp[i] != NULL; i++)
		vfs_opv_init_default(vopvdpp[i]);
}

void
vfs_opv_free(vopvdpp)
	struct vnodeopv_desc **vopvdpp;
{
	int i;

	/*
	 * Free the vectors allocated in vfs_opv_init().
	 */
	for (i = 0; vopvdpp[i] != NULL; i++) {
		/* XXX - shouldn't be M_VNODE */
		free(*(vopvdpp[i]->opv_desc_vector_p), M_VNODE);
		*(vopvdpp[i]->opv_desc_vector_p) = NULL;
	}
}

void
vfs_op_init()
{
	int i;

	DODEBUG(printf("Vnode_interface_init.\n"));
	/*
	 * Figure out how many ops there are by counting the table,
	 * and assign each its offset.
	 */
	for (vfs_opv_numops = 0, i = 0; vfs_op_descs[i]; i++) {
		vfs_op_descs[i]->vdesc_offset = vfs_opv_numops;
		vfs_opv_numops++;
	}
	DODEBUG(printf ("vfs_opv_numops=%d\n", vfs_opv_numops));
}

/*
 * Routines having to do with the management of the vnode table.
 */
struct vattr va_null;

/*
 * Initialize the vnode structures and initialize each file system type.
 */
void
vfsinit()
{
	extern struct vfsops *vfs_list_initial[];
	int i;

	/*
	 * Initialize the namei pathname buffer pool.
	 */
	pool_init(&pnbuf_pool, MAXPATHLEN, 0, 0, 0, "pnbufpl",
	     0, pool_page_alloc_nointr,  pool_page_free_nointr, M_NAMEI);

	/*
	 * Initialize the vnode table
	 */
	vntblinit();

	/*
	 * Initialize the vnode name cache
	 */
	nchinit();

	/*
	 * Initialize the list of vnode operations.
	 */
	vfs_op_init();

	/*
	 * Initialize the special vnode operations.
	 */
	vfs_opv_init(vfs_special_vnodeopv_descs);

	/*
	 * Establish each file system which was statically
	 * included in the kernel.
	 */
	vattr_null(&va_null);
	for (i = 0; vfs_list_initial[i] != NULL; i++) {
		if (vfs_attach(vfs_list_initial[i])) {
			printf("multiple `%s' file systems",
			    vfs_list_initial[i]->vfs_name);
			panic("vfsinit");
		}
	}
}
