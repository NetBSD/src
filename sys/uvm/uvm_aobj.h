/*	$NetBSD: uvm_aobj.h,v 1.5 1998/02/10 02:34:27 perry Exp $	*/

/* copyright here */
/*
 * from: Id: uvm_aobj.h,v 1.1.2.4 1998/02/06 05:19:28 chs Exp
 */

/*
 * uvm_aobj.h
 *
 * anonymous memory uvm_object
 */

#ifndef _UVM_UVM_AOBJ_H_
#define _UVM_UVM_AOBJ_H_

/*
 * flags
 */

/* flags for uao_create: can only be used one time (at bootup) */
#define UAO_FLAG_KERNOBJ	0x1	/* create kernel object */
#define UAO_FLAG_KERNSWAP	0x2	/* enable kernel swap */

/* internal flags */
#define UAO_FLAG_KILLME		0x4	/* aobj should die when last released
					 * page is no longer PG_BUSY ... */
#define UAO_FLAG_NOSWAP		0x8	/* aobj can't swap (kernel obj only!) */

/*
 * prototypes
 */

int uao_set_swslot __P((struct uvm_object *, int, int));

/*
 * globals
 */

extern struct uvm_pagerops aobj_pager;

#endif /* _UVM_UVM_AOBJ_H_ */
