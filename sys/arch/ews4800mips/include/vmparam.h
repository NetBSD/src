/*	$NetBSD: vmparam.h,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

#ifndef _EWS4800MIPS_VMPARAM_H_
#define _EWS4800MIPS_VMPARAM_H_

#include <mips/vmparam.h>

/* VA 0xf0000000-0xffffffff is used for wired_map TLB entries. */
#undef VM_MAX_KERNEL_ADDRESS
#define VM_MAX_KERNEL_ADDRESS		((vaddr_t)0xF0000000)

#define VM_MIN_WIRED_MAP_ADDRESS	((vaddr_t)0xF0000000)
#define VM_MAX_WIRED_MAP_ADDRESS	((vaddr_t)0xFFFFC000)

#define VM_PHYSSEG_MAX		9	/* M0-M8 */

#define VM_NFREELIST		1
#define VM_FREELIST_DEFAULT	0

#ifndef KSEG2IOBUFSIZE
/* reserve PTEs for KSEG2 I/O space */
#define KSEG2IOBUFSIZE		kseg2iobufsize
#endif
extern vsize_t kseg2iobufsize;

#endif /* !_EWS4800MIPS_VMPARAM_H_ */
