/*	$NetBSD: sigcode.S,v 1.1.2.3 2002/11/20 11:02:46 wdk Exp $	*/

#include "opt_compat_ultrix.h"
#include "opt_compat_linux.h"
#include "opt_compat_irix.h"

#include <sys/syscall.h>
#ifdef COMPAT_ULTRIX
#include <compat/ultrix/ultrix_syscall.h>
#endif
#ifdef COMPAT_LINUX
#include <compat/linux/linux_syscall.h>
#endif
#ifdef COMPAT_IRIX
#include <compat/irix/irix_syscall.h>
#endif

#include <mips/asm.h>
#include "assym.h"

	.set	noreorder

/*
 * This code is copied the user's stack for returning from signal handlers
 * (see sendsig() and sigreturn()). We have to compute the address
 * of the sigcontext struct for the sigreturn call.
 *
 * NB: we cannot profile sigcode(), it executes from userspace.
 */
LEAF_NOPROFILE(sigcode)
	move	a0, sp			# address of sigcontext
	li	v0, SYS___sigreturn14	# sigreturn(scp)
	syscall
	break	0			# just in case sigreturn fails
END(sigcode)

/* Upcall "trampoline" code. Invoked from RTE setup by cpu_upcall().
 *
 * *MUST* be after signal trampoline code
 *
 * On entry, stack & registers look like:
 *	
 *      a0	upcall type	
 *      a1	pointer to sa_t array
 *      a2	count of "event" SAs	
 *	a3	count of "interrupted" SAs
 *	t0	arg
 *      t1	address of upcallhandler
 */
LEAF_NOPROFILE(upcallcode)
	jal	t9
	nop				# BD slot
	li	v0, SYS_exit
	li	a0, -1
	syscall
	break	0			# just in case exit fails
END(upcallcode)
XLEAF(esigcode)

XLEAF(esigcode)

#ifdef COMPAT_ULTRIX
LEAF_NOPROFILE(ultrix_sigcode)
	move	a0, sp			# address of sigcontext
	li	v0, ULTRIX_SYS_sigreturn	# sigreturn(scp)
	syscall
	break	0			# just in case sigreturn fails
END(ultrix_sigcode)
XLEAF(ultrix_esigcode)
#endif

#ifdef COMPAT_LINUX
#define SYSCALL_SHIFT 4000		# 4000 shift as in linux_syscall.c
LEAF_NOPROFILE(linux_sigcode)
	move	a0, sp			# address of sigcontext
	li	v0, LINUX_SYS_sigreturn	+ SYSCALL_SHIFT # sigreturn(sf)
	syscall
	break	0			# just in case sigreturn fails
END(linux_sigcode)
XLEAF(linux_esigcode)
#undef SYSCALL_SHIFT
#endif
