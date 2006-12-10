/*	$NetBSD: frameasm.h,v 1.4.20.1 2006/12/10 07:16:43 yamt Exp $	*/
/*	NetBSD: frameasm.h,v 1.4 2004/02/20 17:35:01 yamt Exp 	*/

#ifndef _XEN_FRAMEASM_H_
#define _XEN_FRAMEASM_H_

#include <i386/frameasm.h>

#ifdef _KERNEL_OPT
#include "opt_xen.h"
#endif

/* XXX assym.h */
#define TRAP_INSTR	int $0x82

#if !defined(XEN)
#define	CLI(reg)	cli
#define	STI(reg)	sti
#else
#define XEN_BLOCK_EVENTS(reg)	movb $1,EVTCHN_UPCALL_MASK(reg)
#define XEN_UNBLOCK_EVENTS(reg)	movb $0,EVTCHN_UPCALL_MASK(reg)
#define XEN_TEST_PENDING(reg)	testb $0xFF,EVTCHN_UPCALL_PENDING(reg)

#define CLI(reg)	movl	_C_LABEL(HYPERVISOR_shared_info),reg ;	\
    			XEN_BLOCK_EVENTS(reg)
#define STI(reg)	movl	_C_LABEL(HYPERVISOR_shared_info),reg ;	\
    			XEN_UNBLOCK_EVENTS(reg)
#define STIC(reg)	movl	_C_LABEL(HYPERVISOR_shared_info),reg ;	\
    			XEN_UNBLOCK_EVENTS(reg)  ; \
			testb $0xff,EVTCHN_UPCALL_PENDING(reg)
#endif

#endif /* _XEN_FRAMEASM_H_ */
