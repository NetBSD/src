#include "assym.h"
#include <machine/asm.h>
#include <sys/syscall.h>

	.globl _C_LABEL(netbsd32_esigcode)

/*
 * Signal trampoline; copied to top of user stack.
 */

NENTRY(netbsd32_sigcode)

	.code32
	/*
	 * Handler has returned here as if we called it.  The sigcontext
	 * is on the stack after the 3 args "we" pushed.
	 */
	leal	12(%esp),%eax		# get pointer to sigcontext
	movl	SC_FS32(%eax),%ecx
	movl	SC_GS32(%eax),%edx
	movl	%ecx,%fs
	movl	%edx,%gs
	movl	%eax,4(%esp)		# put it in the argument slot
					# fake return address already there
	movl	$SYS___sigreturn14,%eax
	int	$0x80	 		# enter kernel with args on stack
	movl	$SYS_exit,%eax
	int	$0x80			# exit if sigreturn fails
	.globl	_C_LABEL(esigcode)
_C_LABEL(netbsd32_esigcode):
