/* $NetBSD: __clone.S,v 1.6.10.1 2007/08/28 17:36:30 matt Exp $ */

/*
 * Copyright (c) 2001 Christopher Gilbert
 * All rights reserved.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <machine/asm.h>
#include <sys/errno.h>
#include "SYS.h"

#ifdef WEAK_ALIAS
WEAK_ALIAS(clone, __clone)
#endif

/*
 * int __clone(int (*fn)(void *), void *stack, int flags, void *arg);
 */
ENTRY(__clone)
	/* test stack and function are not null */
#ifdef __thumb__
	cmp	r0, #0
	beq	.Leinval
	cmp	r1, #0
	beq	.Leinval
#else
	teq	r0, #0x00
	teqne	r1, #0x00
	beq	.Leinval
#endif

	/* place the func and its arg onto the child's stack */
#ifdef __thumb__
	sub	r1, #8
	str	r0, [r1, #0]
	str	r3, [r1, #4]
#else
	stmfd	r1!, {r0, r3} 
#endif
	
	/* syscall expects (flags, stack) */
	mov	r0, r2

	SYSTRAP(__clone)
	bcs	.Lcerror

	/*
	 * r1 and r0 are the same as from fork:
	 * r1 == 0 in parent process, r1 == 1 in child process.
	 * r0 == pid of child in parent, r0 == pid of parent in child.
	 */
#ifdef __thumb__
	cmp	r1, #0
	bne	1f
	RET
1:
#else
	teq	r1, #0x00

	/* if this is the parent then just return the pid */
	RETc(eq)
#endif
	/*
	 * This is the child
	 * load the function and arg off the stack
	 */
	XPOP	{r1, r2}

	/* setup return address */
#ifdef __thumb__
	add	r0, pc, #.Lreturnhere - . - 8
	mov	lr, r0
#else
	add	lr, pc, #.Lreturnhere - . - 8
#endif

	/* place arg in r0 */
	mov	r0, r2

	/* call the function */
#ifdef _ARM_ARCH_4T
	bx	r1
#else
	mov	pc, r1
#endif

.Lreturnhere:
	/* call _exit with the returned value */
	bl	PLT_SYM(_C_LABEL(_exit))

	/* NOTREACHED */

	/* error handler if func or stack is NULL */
.Leinval:
	mov	r0, #EINVAL
.Lcerror:
	_SYSCALL_CERROR
END(__clone)
