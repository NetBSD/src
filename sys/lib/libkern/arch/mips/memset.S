/*	$NetBSD: memset.S,v 1.3.18.1 2004/10/19 15:58:08 skrll Exp $	*/
#include <machine/cdefs.h>
#include <mips/asm.h>
#include <machine/endian.h>

#if defined(_LP64)
#define ADDU    daddu
#define SUBU    dsubu
#else   
#define ADDU    addu    
#define SUBU    subu
#endif 

	.set	noreorder


/*
 * memset(void *s1, int c, int len)
 */
LEAF(memset)
	.set noreorder
	blt	a2, 12, smallclr	# small amount to clear?
	move	v0, a0			# save s1 for result

	sll	t1, a1, 8		# compute  c << 8 in t1
	or	t1, t1, a1		# compute c << 8 | c in 11
	sll	t2, t1, 16		# shift that left 16
	or	t1, t2, t1		# or together

	SUBU	t0, zero, a0		# compute # bytes to word align address
	and	t0, t0, 3
	beq	t0, zero, 1f		# skip if word aligned
	SUBU	a2, a2, t0		# subtract from remaining count
	SWHI	t1, 0(a0)		# store 1, 2, or 3 bytes to align
	ADDU	a0, a0, t0
1:
	and	v1, a2, 3		# compute number of whole words left
	SUBU	t0, a2, v1
	SUBU	a2, a2, t0 
	ADDU	t0, t0, a0		# compute ending address
2:
	ADDU	a0, a0, 4		# clear words
#ifdef MIPS3_5900
	nop
	nop
	nop
	nop
#endif	
	bne	a0, t0, 2b		#  unrolling loop does not help
	sw	t1, -4(a0)		#  since we are limited by memory speed

smallclr:
	ble	a2, zero, 2f
	ADDU	t0, a2, a0		# compute ending address
1:
	ADDU	a0, a0, 1		# clear bytes
#ifdef MIPS3_5900	
	nop
	nop
	nop
	nop
#endif	
	bne	a0, t0, 1b
	sb	a1, -1(a0)
2:
	j	ra
	nop
	.set reorder
END(memset)
