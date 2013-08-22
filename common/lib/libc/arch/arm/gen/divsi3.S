/*	$NetBSD: divsi3.S,v 1.8 2013/08/22 19:25:00 matt Exp $	*/

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

ENTRY(__divsi3)
#if defined(__ARM_ARCH_EXT_IDIV__)
# if defined(__ARM_EABI__)
	mov	r3, r0		@ save for mls
# endif
	sdiv	r0, r0, r1
# if defined(__ARM_EABI__)
	mls	r1, r0, r1, r3	@ return modulus in r1
# endif
	RET
#elif !defined(__thumb__) || defined(_ARM_ARCH_T2)
	b	__divide
#else
#if defined(__ARM_EABI__) && (!defined(_KERNEL) && !defined(_STANDALONE))
	.fnstart
	.cfi_startproc
#endif
	push	{r4, lr}
#if defined(__ARM_EABI__) && (!defined(_KERNEL) && !defined(_STANDALONE))
	.cfi_def_cfa_offset 8
	.cfi_offset 14, -4
	.cfi_offset 4, -8
#endif
	bl	__divide
	pop	{r4, pc}
#ifdef __ARM_EABI__
	.cfi_endproc
	.fnend
#endif
#endif
END(__divsi3)

#if defined(__ARM_EABI__)
STRONG_ALIAS(__aeabi_idivmod, __divsi3)
STRONG_ALIAS(__aeabi_idiv, __divsi3)
#if defined(PIC_SYMVER)
	.symver	__aeabi_idiv,__aeabi_idiv@@GCC_3.5
	.symver	__aeabi_idivmod,__aeabi_idivmod@@GCC_3.5
#endif
#endif
