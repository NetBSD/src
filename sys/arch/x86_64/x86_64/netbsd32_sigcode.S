#include "assym.h"
#include <machine/asm.h>
#include <sys/syscall.h>

	.globl _C_LABEL(netbsd32_esigcode)

/*
 * Signal trampoline; copied to top of user stack.
 */

NENTRY(netbsd32_sigcode)

	.code32

	call	*SIGF_HANDLER32(%esp)
	leal	SIGF_SC32(%esp),%eax	# scp (the call may have clobbered the
					# copy at SIGF_SCP(%esp))
	movl	SC_FS32(%eax),%ecx
	movl	SC_GS32(%eax),%edx
	movl	%ecx,%fs
	movl	%edx,%gs
	pushl	%eax
	pushl	%eax			# junk to fake return address
	movl	$SYS___sigreturn14,%eax
	int	$0x80	 		# enter kernel with args on stack
	movl	$SYS_exit,%eax
	int	$0x80			# exit if sigreturn fails
	.globl	_C_LABEL(esigcode)
_C_LABEL(netbsd32_esigcode):
