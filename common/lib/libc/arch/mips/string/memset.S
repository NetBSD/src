/*	$NetBSD: memset.S,v 1.1.40.1 2009/08/16 03:02:47 matt Exp $	*/
#include <machine/cdefs.h>
#include <mips/asm.h>

#ifndef _LOCORE		/* avoid tripping over __RENAME() */
#define _LOCORE		/* XXX not really, just assembly-code source */
#endif

#include <machine/endian.h>

#if defined(SYSLIBC_SCCS) && !defined(lint)
	RCSID("$NetBSD: memset.S,v 1.1.40.1 2009/08/16 03:02:47 matt Exp $")
#endif /* SYSLIBC_SCCS and not lint */

	.set	noreorder

/*
 * memset(void *s1, int c, int len)
 */
LEAF(memset)
	blt		a2, 12, smallclr # small amount to clear?
	move		v0, a0		# save s1 for result

	sll		t1, a1, 8	# compute  c << 8 in t1
	or		t1, t1, a1	# compute c << 8 | c in t1
	sll		t2, t1, 16	# shift that left 16
	or		t1, t2, t1	# or together
#if SZREG == 8
	dsll32		t2, t1, 0	# shift that left 32
	or		t1, t2, t1	# or together
#endif

	subu		t0, zero, a0	# compute # bytes to word align address
	and		t0, t0, SZREG-1
	beq		t0, zero, 1f	# skip if word aligned
	PTR_SUBU	a2, a2, t0	# subtract from remaining count
#if 1
	SWHI		t1, 0(a0)	# store 1, 2, or 3 bytes to align
	PTR_ADDU	a0, a0, t0
#if SZREG == 8
	sltui		t0, t0, 4	# did we need to store more than 3?
	bne		t0, zero, 1f	# nope, 
	nop
	sw		t1, -4(a0)	# store remaining 4 bytes
#endif
#else
/*
 * 
 */
	andi		t2, a0, SZREG-1	# v0 + t0 = SZREG
	PTR_ADDU	a0, a0, t0	# advance to word boundary
	REG_L		a3, -SZREG(a0)	# load previous word
	sll		t2, t2, 3	# convert byte count to bit count
	sll		t0, t0, 3	# convert byte count to bit count
#if _BYTE_ORDER == _BIG_ENDIAN
	REG_SLLV	v1, t1, t2	# clear msb bits of fill
	REG_SRLV	v1, v1, t2
	REG_SRLV	a3, a3, t0	# clear lsb bits of source
	REG_SLLV	a3, a3, t0
#elif _BYTE_ORDER == _LITTLE_ENDIAN
	REG_SRLV	v1, t1, t2	# clear lsb bits of fill
	REG_SLLV	v1, v1, t2
	REG_SLLV	a3, a3, t0	# clear msb bits of source
	REG_SRLV	a3, a3, t0
#endif
	or		a3, a3, v1	# merge source and fill
	REG_S		a3, -SZREG(a0)	# store it
#endif /* 0 */
1:
	and		v1, a2, SZREG-1	# compute number of whole words left
	PTR_SUBU	t0, a2, v1
	PTR_SUBU	a2, a2, t0 
	PTR_ADDU	t0, t0, a0	# compute ending address
2:
	PTR_ADDU	a0, a0, SZREG	# clear words
	bne		a0, t0, 2b	#  unrolling loop does not help
	REG_S		t1, -SZREG(a0)	#  since we are limited by memory speed

smallclr:
	ble		a2, zero, 2f
	PTR_ADDU	t0, a2, a0		# compute ending address
1:
	PTR_ADDU	a0, a0, 1		# clear bytes
	bne		a0, t0, 1b
	sb		a1, -1(a0)
2:
	j		ra
	nop
END(memset)
