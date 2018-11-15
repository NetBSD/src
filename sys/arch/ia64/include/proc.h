/*	$NetBSD: proc.h,v 1.8 2018/11/15 20:06:23 scole Exp $	*/

#ifndef _IA64_PROC_H_
#define _IA64_PROC_H_

#include <machine/frame.h>

/*
 * Process u-area is organised as follows:
 *
 *   ------------------------------------------- 
 *  |                      |         |    |     |
 *  |  bspstore       sp   | 16bytes | TF | PCB |
 *  |  ---->        <---   |         |    |     |
 *   -------------------------------------------
 *              -----> Higher Addresses
 */

/*
 * Machine-dependent part of the lwp structure for ia64
 */
struct mdlwp {
	u_long	md_flags;
	struct	trapframe *md_tf;	/* trap/syscall registers */
	__volatile int md_astpending;	/* AST pending for this process */
	void *user_stack;
	uint64_t user_stack_size;
};

/*
 * md_flags usage
 * --------------
 * XXX:
 */

struct mdproc {
  /* XXX: Todo */
	void	(*md_syscall)(struct lwp *, u_int64_t, struct trapframe *);
					/* Syscall handling function */
};

#define UAREA_PCB_OFFSET	(USPACE - sizeof(struct pcb))
#define UAREA_TF_OFFSET		(UAREA_PCB_OFFSET - sizeof(struct trapframe))
#define UAREA_SP_OFFSET		(UAREA_TF_OFFSET -16 -sizeof(uint64_t))
#define UAREA_BSPSTORE_OFFSET	(0)
#define UAREA_STACK_SIZE	(USPACE - 16 - sizeof(struct trapframe) - \
				 sizeof(struct pcb))

#endif /* _IA64_PROC_H_ */
