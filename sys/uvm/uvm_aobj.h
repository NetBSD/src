/*	$Id: uvm_aobj.h,v 1.1.1.1 1998/02/05 06:25:10 mrg Exp $	*/

/* copyright here */

/*
 * uvm_aobj.h
 *
 * anonymous memory uvm_object
 */

#ifndef UVM_UVM_AOBJ_H
#define UVM_UVM_AOBJ_H

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
#endif
