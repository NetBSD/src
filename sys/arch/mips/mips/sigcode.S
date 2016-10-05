/*	$NetBSD: sigcode.S,v 1.4.32.1 2016/10/05 20:55:32 skrll Exp $	*/

#include "opt_compat_netbsd.h"
#include "opt_compat_ultrix.h"
#include "opt_compat_linux.h"

#include <mips/signal.h>
#include <sys/syscall.h>
#ifdef COMPAT_ULTRIX
#include <compat/ultrix/ultrix_syscall.h>
#endif
#ifdef COMPAT_LINUX
#include <compat/linux/linux_syscall.h>
#endif

#include <mips/asm.h>

RCSID("$NetBSD: sigcode.S,v 1.4.32.1 2016/10/05 20:55:32 skrll Exp $")

#include "assym.h"

	.set	noreorder

#ifdef COMPAT_16
/*
 * This code is copied the user's stack for returning from signal handlers
 * (see sendsig() and sigreturn()). We have to compute the address
 * of the sigcontext struct for the sigreturn call.
 *
 * NB: we cannot profile sigcode(), it executes from userspace.
 */
LEAF_NOPROFILE(sigcode)
	move	a0, sp			# address of sigcontext
	li	v0, SYS_compat_16___sigreturn14	# sigreturn(scp)
	syscall
	break	0			# just in case sigreturn fails
END(sigcode)
XLEAF(esigcode)
#endif

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
