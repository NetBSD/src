#include <machine/asm.h>
#include "assym.h"

# %eax = sum
# %ebx = m
# %cl = rotate
# %edx = len
# %esi = mlen
# %ebp = w

#define	SWAP \
	roll	$8, %eax	; \
	xorb	$8, %cl

#define	UNSWAP \
	roll	%cl, %eax

#define	MOP \
	adcl	$0, %eax

#define	ADVANCE(n) \
	leal	n(%ebp), %ebp	; \
	leal	-n(%esi), %esi	; \

#define	ADDBYTE \
	SWAP			; \
	addb	(%ebp), %ah

#define	ADDWORD \
	addw	(%ebp), %ax

#define	REDUCE \
	movzwl	%ax, %esi	; \
	shrl	$16, %eax	; \
	addw	%si, %ax	; \
	adcw	$0, %ax

#define	ADD(n) \
	addl	n(%ebp), %eax

#define	ADC(n) \
	adcl	n(%ebp), %eax

ENTRY(myc2_in_cksum)
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	/*pushl	%edi*/

	movl	16(%esp), %ebx
	movl	20(%esp), %edx
	xorl	%eax, %eax
	xorb	%cl, %cl

mbuf_loop_1:
	testl	%edx, %edx
	jz	done

mbuf_loop_2:
	testl	%ebx, %ebx
	jz	out_of_mbufs

	movl	M_DATA(%ebx), %ebp
	movl	M_LEN(%ebx), %esi
	movl	M_NEXT(%ebx), %ebx

	cmpl	%edx, %esi
	jbe	1f
	movl	%edx, %esi

1:
	subl	%esi, %edx

	cmpl	$16, %esi
	jb	short_mbuf

	testl	$3, %ebp
	jz	dword_aligned

	testl	$1, %ebp
	jz	byte_aligned

	ADDBYTE
	ADVANCE(1)
	MOP

	testl	$2, %ebp
	jz	word_aligned

byte_aligned:
	ADDWORD
	ADVANCE(2)
	MOP

word_aligned:
dword_aligned:
	testl	$4, %ebp
	jnz	qword_aligned

	ADD(0)
	ADVANCE(4)
	MOP

qword_aligned:
	testl	$8, %ebp
	jz	oword_aligned

	ADD(0)
	ADC(4)
	ADVANCE(8)
	MOP

oword_aligned:
	subl	$128, %esi
	jb	finished_128

loop_128:
	ADD(12)
	ADC(0)
	ADC(4)
	ADC(8)
	ADC(28)
	ADC(16)
	ADC(20)
	ADC(24)
	ADC(44)
	ADC(32)
	ADC(36)
	ADC(40)
	ADC(60)
	ADC(48)
	ADC(52)
	ADC(56)
	ADC(76)
	ADC(64)
	ADC(68)
	ADC(72)
	ADC(92)
	ADC(80)
	ADC(84)
	ADC(88)
	ADC(108)
	ADC(96)
	ADC(100)
	ADC(104)
	ADC(124)
	ADC(112)
	ADC(116)
	ADC(120)
	leal	128(%ebp), %ebp
	MOP

	subl	$128, %esi
	jnb	loop_128

finished_128:
	addl	$128, %esi

	subl	$32, %esi
	jb	finished_32

loop_32:
	ADD(12)
	ADC(0)
	ADC(4)
	ADC(8)
	ADC(28)
	ADC(16)
	ADC(20)
	ADC(24)
	leal	32(%ebp), %ebp
	MOP

	subl	$32, %esi
	jnb	loop_32

finished_32:
	testl	$16, %esi
	jz	finished_16

	ADD(12)
	ADC(0)
	ADC(4)
	ADC(8)
	leal	16(%ebp), %ebp
	MOP

finished_16:
short_mbuf:
	testl	$8, %esi
	jz	finished_8

	ADD(0)
	ADC(4)
	leal	8(%ebp), %ebp
	MOP

finished_8:
	testl	$4, %esi
	jz	finished_4

	ADD(0)
	leal	4(%ebp), %ebp
	MOP

finished_4:
	testl	$3, %esi
	jz	mbuf_loop_1

	testl	$2, %esi
	jz	finished_2

	ADDWORD
	leal	2(%ebp), %ebp
	MOP

	testl	$1, %esi
	jz	finished_1

finished_2:
	ADDBYTE
	MOP

finished_1:
mbuf_done:
	testl	%edx, %edx
	jnz	mbuf_loop_2

done:
	UNSWAP
	REDUCE
	notw	%ax

return:
	/*popl	%edi*/
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret

out_of_mbufs:
	pushl	$1f
	call	_printf
	leal	4(%esp), %esp
	jmp	return
1:
	.asciz	"cksum: out of data\n"
