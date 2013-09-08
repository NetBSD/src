/*	$NetBSD: udivsi3.S,v 1.6 2013/09/08 13:24:16 matt Exp $	*/

/*
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <machine/asm.h>

#if defined(__ARM_EABI__) && defined(__thumb__) && !defined(_ARM_ARCH_T2)
ARM_ENTRY(__udivsi3)
#else
ENTRY(__udivsi3)
#endif
#if defined(__ARM_ARCH_EXT_IDIV__)
# if defined(__ARM_EABI__)
	mov	r3, r0		@ save for mls
# endif
	udiv	r0, r0, r1
# if defined(__ARM_EABI__)
	mls	r1, r0, r1, r3	@ return modulus in r1
# endif
	RET
#elif defined(__ARM_EABI__) && defined(_LIBC)
#ifdef _ARM_ARCH_7
	movw	r2, #:lower16:REL_SYM(__libc_arm_hwdiv_present, .LPIC0)
	movt	r2, #:upper16:REL_SYM(__libc_arm_hwdiv_present, .LPIC0)
#else
	ldr	r2, .Lhwdiv_present
#endif
#ifdef PIC
	add	r2, r2, pc
#endif
	ldr	r2, [r2]
.LPIC0:	cmp	r2, #0
	beq	__udivide
	mov	r3, r0
#if defined(__ARM_ARCH_EXT_IDIV__)
	udiv	r0, r0, r1
	mls	r1, r0, r1, r3	/* return modulus in r1 */
#elif defined(__thumb__) && defined(_ARM_ARCH_T2)
	.inst.w 0xfbb0f0f1
	.inst.w	0xfb003111
#else
	.inst	0xe730f110
	.inst	0xe0613190
#endif
	RET
#ifndef _ARM_ARCH_7
	.align 0
.Lhwdiv_present:
	.word	REL_SYM(__libc_arm_hwdiv_present, .LPIC0)
#endif
#else
	b	__udivide
#endif
END(__udivsi3)

#ifdef __ARM_EABI__
STRONG_ALIAS(__aeabi_uidivmod, __udivsi3)
STRONG_ALIAS(__aeabi_uidiv, __udivsi3)
#endif
