/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: locore.s,v 1.2 1994/08/16 23:47:31 ragge Exp $
 */

 /* All bugs are subject to removal without further notice */
		


#include "vax/include/mtpr.h"
#include "vax/include/param.h"
#include "vax/include/loconf.h"
#include "vax/include/vmparam.h"
/* #include "assym.s" */
#include "vax/include/pte.h"
#include "vax/include/nexus.h"
#include "sys/syscall.h"
#include "sys/errno.h"

/*** MACROS **************************************************************/

#define ROUND_VAX_PAGE(x)		 \
		addl2 	$NBPG-1,x	;\
		bicl2	$NBPG-1,x	;

#define ABORT(x)			 \
		pushal	1f		;\
		calls	$1,_printf	;\
		halt			;\
1:		.asciz	x
		
/*** CODE ****************************************************************/

		.text
		.globl start

start:		.globl start
		.word 0x0			# Save nothing.

		movl	r10,_bootdev		# Bootdev from boot
		movl	r11,_boothowto		# Boothow from boot

		mtpr    $0x1f,$PR_IPL   	# Disable all interrupts.
		mtpr	$0,$PR_MAPEN		# Disable memory mapping.
		mtpr	$0,$PR_SCBB		# Put interrupt vectors
						# at physical address 0.
		pushl	$0x00000000
		pushl	$to_kmem+0x80000000

		rei				# Jump to kernel area.
						# Kind of cheating, as we are
						# not running mapped, but it
						# saves us from writing
						# relocatable code.
to_kmem:

		mtpr	$0,$PR_P0LR
		mtpr    $0,$PR_P0BR
		mtpr    $0,$PR_P1LR
		mtpr    $0x80000000,$PR_P1BR

set_text:	movl	$0x0,r0			# Text starts at physical 
						# address 0.
		movl	r0,p_text
		movl	$_etext-0x80000000,r1	# Calculate size of text.
		movl	r1,l_text

set_data:	addl2	r1,r0			# Calculate physical data 
						# address.
		movl	r0,p_data

		movl	r0,r1			# Check that data is 
						# page aligned.
		ROUND_VAX_PAGE(r0)		# If not, abort.
		cmpl	r0,r1
		beql	set_data_siz
		ABORT("Data segment must be page aligned (linker problem)\n")

set_data_siz:	subl3	$_etext,$_end,r1	# Calculate size of data.
/*		movl	$(_end-_etext),r1	# Calculate size of data. */
		ROUND_VAX_PAGE(r1)		# Page align data size.
		movl	r1,l_data

		clrl	r3			# Clear bss
		movl	$_edata,r4
1:		subl3   r4, $_end,r2
		clrb	(r4)
		incl	r4
		aoblss	r2,r3,1b

set_prot1:	addl2	r1,r0			# Calculate physical 
						# prot1 address.
		movl	r0,p_prot1
		movl	$NBPG,r1		# Calculate prot1 size.
		movl	r1,l_prot1

set_stack:	addl2	r1,r0			# One page with no access.
		movl	r0,p_stack
		movl	$ISTACK_SIZE,r1		# Calculate stack size.
		movl	r1,l_stack

set_prot2:	addl2   r1,r0			# Calculate physical 
						# prot2 address.
		bisl3	$0x80000000,r0,r1	# Calculate virtual 
						# stack address.
                mtpr    r1,$PR_ISP              # Set Interrupt Stack.
		movl	r0,p_prot2
		movl	$NBPG,l_prot2

		addl2	$NBPG,r0		# Set interrupt stack pointer 
		bisl3	$0x80000000,r0,r1	# Calculate virtual 
						# process entry address.
		movl	r1,_proc0paddr		# in the middle of a page.

		addl2	$NBPG*UPAGES,r0		# Reserve proc0 block.

		movl	r0,p_prot3
		movl	$NBPG,l_prot3

		bisl3	$0x80000000,r0,r1	# Calculate virtual 
						# stack address.
                mtpr    r1,$PR_KSP		# with no access (to make
						# sure it is never used).

set_cmap:	addl2	$NBPG,r0		# Calculate physical 
						# cmap address.
		movl	r0,p_cmap
		bisl3	$0x80000000,r0,v_cmap	# Calculate virtual 
						# cmap address.
		movl	$2*NBPG,r1		# Calculate cmap size.
		movl	r1,l_cmap

		addl2	r1,r0			# Calculate pmap address.
		movl	r0,p_ptab

  /*
   *  Calculate memory size
   *	We check 1 long every physical page, to see if there is any memory 
   *	there. When there's no more memory we will get a V_MACHINE_CHK 
   *	interrupt, which clears memtest and returns to the next instruction.
   */

memory_test:	movl    $2f,_memtest            # Start memory test.
                movl    $0,r0                   # Start the test at ADDR 0x0.
1:              movl    (r0),r1                 # If not valid Interrupt.
               				        # XXX The interrupt will
                addl2   $NBPG,r0	        # Jump one page forward.
		brb	1b
2:		movl	$0,mem_tab		# VAX has contig memory, so only
		movl	r0,mem_tab+4		# one memlist entry needed.
		ashl	$9,$VM_KERNEL_PT_PAGES,l_ptab
		addl3	p_ptab,l_ptab,_phys_avail

/*
 *  Initialize virtual memory.
 *  Set physical/virtual memory limits.
 */

		pushl	$mem_tab		# Push pointer to memory table.
		calls	$1,_pmap_bootstrap	# Set physical memory

/*
 *  Start virtual memory.
 */

		mtpr	_ptab_addr,$PR_SBR	# Set ptab start address
		mtpr	_ptab_len,$PR_SLR	# Set ptab length
		mtpr	$0,$PR_TBIA		# Flush translation buffer
		mtpr	$1,$PR_MAPEN		# Enable memory mapping

/*
 *  Get CPU type for later use.
 */

		mfpr	$PR_SID,_cpu_type

/*
 *  Start 'high level kernel'. Should return in init.
 */

                calls   $0,_main                # Calls main.

/*
 *  If we get here, we're in init. go to usermode!
 */

		mtpr	$0x80000000,$PR_USP	# Set user stack to top.
		pushl	$0x3c00000
		pushl	$2			# Start at 2 in icode
		rei

/*
 *  End of locore startup
 */

		.globl	__spl
__spl:		.word 0x0
		mfpr	$PR_IPL,r0
		mtpr    4(ap),$PR_IPL
		ret

		.globl _TBIS
_TBIS:		.word 0x0
		mtpr	4(ap),$58
		ret

		.globl _TBIA
_TBIA:		.word 0x0
		mtpr	4(ap),$PR_TBIA
		ret

		.globl _DCIA
_DCIA:		.word 0x0
						# XXX Currently a no-op.
		ret

		.globl _DCIS
_DCIS:		.word 0x0
						# XXX Currently a no-op.
		ret

		.globl _DCIU
_DCIU:		.word 0x0
						# XXX Currently a no-op.
		ret
		
		.globl _spl0
_spl0:		.word 0x0
		mtpr	$0,$PR_IPL		# Supposed to work...
		ret

		.globl _physcopypage
_physcopypage:	.word 0x7
		movl	4(ap),r0
		ashl	$-PGSHIFT,r0,r0
		bisl2	$(PG_V|PG_RO),r0

		movl	8(ap),r1
		ashl    $-PGSHIFT,r1,r1
		bisl2   $(PG_V|PG_KW),r1

		movl	r0,*(_pte_cmap)
		movl	r1,*$4+(_pte_cmap)

		movl	v_cmap,r2
		addl3	$0x200,r2,r1
		mtpr	r1,$PR_TBIS
		mtpr	r2,$PR_TBIS

		movl	r1,r0
1:		movl	(r2)+,(r1)+
		cmpl	r0,r2
		bneq	1b
		ret



		.globl _clearpage
_clearpage:	.word 0x3
		movl	4(ap),r0
		ashl	$-PGSHIFT,r0,r0
		bisl2	$(PG_V|PG_KW),r0
		movl	r0,*(_pte_cmap)
		movl	v_cmap,r0
		mtpr	r0,$PR_TBIS
		addl3	$0x200,r0,r1
1:		clrl	(r0)+
		cmpl	r0,r1
		bneq	1b
		ret


		.globl _badaddr
_badaddr:	.word	0xE		# XXX What to save?
					# Called with addr,b/w/l
		pushl	$0x1f		# XXX Should be a define
		calls	$1,__spl	# Increase IPL to 0x1f
		movl	4(ap),r2 	# First argument, the address
		movl	8(ap),r1 	# Sec arg, b,w,l
		pushl	r0		# Save old IPL

		movl	$4f,_memtest	# Set the return adress

		caseb	r1,$1,$4	# What is the size
1:		.word	1f-1b		
		.word	2f-1b
		.word	3f-1b		# This is unused
		.word	3f-1b
		
1:		movb	(r2),r1		# Test a byte
		movl	$0,r3
		brb	5f

2:		movw	(r2),r1		# Test a word
		movl	$0,r3
		brb	5f

3:		movl	(r2),r1		# Test a long
		movl	$0,r3
		brb	5f

4:		movl	$1,r3		# Got machine chk => addr bad
5:		calls	$1,__spl	# Reset IPL
		movl	r3,r0
		ret

		.align 2
_mba_0:		.globl _mba_0
		movl	$0,mbanum
		brb	_mba
		.align 2
_mba_1:		.globl _mba_1
		movl	$1,mbanum
		brb	_mba
		.align 2
_mba_2:		.globl _mba_2
		movl	$2,mbanum
		brb	_mba
		.align 2
_mba_3:		.globl _mba_3
		movl	$3,mbanum
_mba:		pushr	$0xffff
		pushl	mbanum
		calls	$1,_mbainterrupt
		popr	$0xffff
		rei

		.align	2
_uba_00:	.globl	_uba_00
		movl	$0,ubanum
		jmp	_uba
		.align	2
_uba_01:	.globl	_uba_01
		movl	$1,ubanum
		jmp	_uba
		.align	2
_uba_02:	.globl	_uba_02
		movl	$2,ubanum
		jmp	_uba
		.align	2
_uba_03:	.globl	_uba_03
		movl	$3,ubanum
		jmp	_uba
		.align	2
_uba_04:	.globl	_uba_04
		movl	$4,ubanum
		jmp	_uba
		.align	2
_uba_05:	.globl	_uba_05
		movl	$5,ubanum
		jmp	_uba
		.align	2
_uba_06:	.globl	_uba_06
		movl	$6,ubanum
		jmp	_uba
		.align	2
_uba_07:	.globl	_uba_07
		movl	$7,ubanum
		jmp	_uba
		.align	2
_uba_08:	.globl	_uba_08
		movl	$8,ubanum
		jmp	_uba
		.align	2
_uba_09:	.globl	_uba_09
		movl	$9,ubanum
		jmp	_uba
		.align	2
_uba_0a:	.globl	_uba_0a
		movl	$10,ubanum
		jmp	_uba
		.align	2
_uba_0b:	.globl	_uba_0b
		movl	$11,ubanum
		jmp	_uba
		.align	2
_uba_0c:	.globl	_uba_0c
		movl	$12,ubanum
		jmp	_uba
		.align	2
_uba_0d:	.globl	_uba_0d
		movl	$13,ubanum
		jmp	_uba
		.align	2
_uba_0e:	.globl	_uba_0e
		movl	$14,ubanum
		jmp	_uba
		.align	2
_uba_0f:	.globl	_uba_0f
		movl	$15,ubanum
		jmp	_uba
		.align	2
_uba_10:	.globl	_uba_10
		movl	$16,ubanum
		jmp	_uba
		.align	2
_uba_11:	.globl	_uba_11
		movl	$17,ubanum
		jmp	_uba
		.align	2
_uba_12:	.globl	_uba_12
		movl	$18,ubanum
		jmp	_uba
		.align	2
_uba_13:	.globl	_uba_13
		movl	$19,ubanum
		jmp	_uba
		.align	2
_uba_14:	.globl	_uba_14
		movl	$20,ubanum
		jmp	_uba
		.align	2
_uba_15:	.globl	_uba_15
		movl	$21,ubanum
		jmp	_uba
		.align	2
_uba_16:	.globl	_uba_16
		movl	$22,ubanum
		jmp	_uba
		.align	2
_uba_17:	.globl	_uba_17
		movl	$23,ubanum
		jmp	_uba
		.align	2
_uba_18:	.globl	_uba_18
		movl	$24,ubanum
		jmp	_uba
		.align	2
_uba_19:	.globl	_uba_19
		movl	$25,ubanum
		jmp	_uba
		.align	2
_uba_1a:	.globl	_uba_1a
		movl	$26,ubanum
		jmp	_uba
		.align	2
_uba_1b:	.globl	_uba_1b
		movl	$27,ubanum
		jmp	_uba
		.align	2
_uba_1c:	.globl	_uba_1c
		movl	$28,ubanum
		jmp	_uba
		.align	2
_uba_1d:	.globl	_uba_1d
		movl	$29,ubanum
		jmp	_uba
		.align	2
_uba_1e:	.globl	_uba_1e
		movl	$30,ubanum
		jmp	_uba
		.align	2
_uba_1f:	.globl	_uba_1f
		movl	$31,ubanum
		jmp	_uba
		.align	2
_uba_20:	.globl	_uba_20
		movl	$32,ubanum
		jmp	_uba
		.align	2
_uba_21:	.globl	_uba_21
		movl	$33,ubanum
		jmp	_uba
		.align	2
_uba_22:	.globl	_uba_22
		movl	$34,ubanum
		jmp	_uba
		.align	2
_uba_23:	.globl	_uba_23
		movl	$35,ubanum
		jmp	_uba
		.align	2
_uba_24:	.globl	_uba_24
		movl	$36,ubanum
		jmp	_uba
		.align	2
_uba_25:	.globl	_uba_25
		movl	$37,ubanum
		jmp	_uba
		.align	2
_uba_26:	.globl	_uba_26
		movl	$38,ubanum
		jmp	_uba
		.align	2
_uba_27:	.globl	_uba_27
		movl	$39,ubanum
		jmp	_uba
		.align	2
_uba_28:	.globl	_uba_28
		movl	$40,ubanum
		jmp	_uba
		.align	2
_uba_29:	.globl	_uba_29
		movl	$41,ubanum
		jmp	_uba
		.align	2
_uba_2a:	.globl	_uba_2a
		movl	$42,ubanum
		jmp	_uba
		.align	2
_uba_2b:	.globl	_uba_2b
		movl	$43,ubanum
		jmp	_uba
		.align	2
_uba_2c:	.globl	_uba_2c
		movl	$44,ubanum
		jmp	_uba
		.align	2
_uba_2d:	.globl	_uba_2d
		movl	$45,ubanum
		jmp	_uba
		.align	2
_uba_2e:	.globl	_uba_2e
		movl	$46,ubanum
		jmp	_uba
		.align	2
_uba_2f:	.globl	_uba_2f
		movl	$47,ubanum
		jmp	_uba
		.align	2
_uba_30:	.globl	_uba_30
		movl	$48,ubanum
		jmp	_uba
		.align	2
_uba_31:	.globl	_uba_31
		movl	$49,ubanum
		jmp	_uba
		.align	2
_uba_32:	.globl	_uba_32
		movl	$50,ubanum
		jmp	_uba
		.align	2
_uba_33:	.globl	_uba_33
		movl	$51,ubanum
		jmp	_uba
		.align	2
_uba_34:	.globl	_uba_34
		movl	$52,ubanum
		jmp	_uba
		.align	2
_uba_35:	.globl	_uba_35
		movl	$53,ubanum
		jmp	_uba
		.align	2
_uba_36:	.globl	_uba_36
		movl	$54,ubanum
		jmp	_uba
		.align	2
_uba_37:	.globl	_uba_37
		movl	$55,ubanum
		jmp	_uba
		.align	2
_uba_38:	.globl	_uba_38
		movl	$56,ubanum
		jmp	_uba
		.align	2
_uba_39:	.globl	_uba_39
		movl	$57,ubanum
		jmp	_uba
		.align	2
_uba_3a:	.globl	_uba_3a
		movl	$58,ubanum
		jmp	_uba
		.align	2
_uba_3b:	.globl	_uba_3b
		movl	$59,ubanum
		jmp	_uba
		.align	2
_uba_3c:	.globl	_uba_3c
		movl	$60,ubanum
		jmp	_uba
		.align	2
_uba_3d:	.globl	_uba_3d
		movl	$61,ubanum
		jmp	_uba
		.align	2
_uba_3e:	.globl	_uba_3e
		movl	$62,ubanum
		jmp	_uba
		.align	2
_uba_3f:	.globl	_uba_3f
		movl	$63,ubanum
		jmp	_uba
		.align	2
_uba_40:	.globl	_uba_40
		movl	$64,ubanum
		jmp	_uba
		.align	2
_uba_41:	.globl	_uba_41
		movl	$65,ubanum
		jmp	_uba
		.align	2
_uba_42:	.globl	_uba_42
		movl	$66,ubanum
		jmp	_uba
		.align	2
_uba_43:	.globl	_uba_43
		movl	$67,ubanum
		jmp	_uba
		.align	2
_uba_44:	.globl	_uba_44
		movl	$68,ubanum
		jmp	_uba
		.align	2
_uba_45:	.globl	_uba_45
		movl	$69,ubanum
		jmp	_uba
		.align	2
_uba_46:	.globl	_uba_46
		movl	$70,ubanum
		jmp	_uba
		.align	2
_uba_47:	.globl	_uba_47
		movl	$71,ubanum
		jmp	_uba
		.align	2
_uba_48:	.globl	_uba_48
		movl	$72,ubanum
		jmp	_uba
		.align	2
_uba_49:	.globl	_uba_49
		movl	$73,ubanum
		jmp	_uba
		.align	2
_uba_4a:	.globl	_uba_4a
		movl	$74,ubanum
		jmp	_uba
		.align	2
_uba_4b:	.globl	_uba_4b
		movl	$75,ubanum
		jmp	_uba
		.align	2
_uba_4c:	.globl	_uba_4c
		movl	$76,ubanum
		jmp	_uba
		.align	2
_uba_4d:	.globl	_uba_4d
		movl	$77,ubanum
		jmp	_uba
		.align	2
_uba_4e:	.globl	_uba_4e
		movl	$78,ubanum
		jmp	_uba
		.align	2
_uba_4f:	.globl	_uba_4f
		movl	$79,ubanum
		jmp	_uba
		.align	2
_uba_50:	.globl	_uba_50
		movl	$80,ubanum
		jmp	_uba
		.align	2
_uba_51:	.globl	_uba_51
		movl	$81,ubanum
		jmp	_uba
		.align	2
_uba_52:	.globl	_uba_52
		movl	$82,ubanum
		jmp	_uba
		.align	2
_uba_53:	.globl	_uba_53
		movl	$83,ubanum
		jmp	_uba
		.align	2
_uba_54:	.globl	_uba_54
		movl	$84,ubanum
		jmp	_uba
		.align	2
_uba_55:	.globl	_uba_55
		movl	$85,ubanum
		jmp	_uba
		.align	2
_uba_56:	.globl	_uba_56
		movl	$86,ubanum
		jmp	_uba
		.align	2
_uba_57:	.globl	_uba_57
		movl	$87,ubanum
		jmp	_uba
		.align	2
_uba_58:	.globl	_uba_58
		movl	$88,ubanum
		jmp	_uba
		.align	2
_uba_59:	.globl	_uba_59
		movl	$89,ubanum
		jmp	_uba
		.align	2
_uba_5a:	.globl	_uba_5a
		movl	$90,ubanum
		jmp	_uba
		.align	2
_uba_5b:	.globl	_uba_5b
		movl	$91,ubanum
		jmp	_uba
		.align	2
_uba_5c:	.globl	_uba_5c
		movl	$92,ubanum
		jmp	_uba
		.align	2
_uba_5d:	.globl	_uba_5d
		movl	$93,ubanum
		jmp	_uba
		.align	2
_uba_5e:	.globl	_uba_5e
		movl	$94,ubanum
		jmp	_uba
		.align	2
_uba_5f:	.globl	_uba_5f
		movl	$95,ubanum
		jmp	_uba
		.align	2
_uba_60:	.globl	_uba_60
		movl	$96,ubanum
		jmp	_uba
		.align	2
_uba_61:	.globl	_uba_61
		movl	$97,ubanum
		jmp	_uba
		.align	2
_uba_62:	.globl	_uba_62
		movl	$98,ubanum
		jmp	_uba
		.align	2
_uba_63:	.globl	_uba_63
		movl	$99,ubanum
		jmp	_uba
		.align	2
_uba_64:	.globl	_uba_64
		movl	$100,ubanum
		jmp	_uba
		.align	2
_uba_65:	.globl	_uba_65
		movl	$101,ubanum
		jmp	_uba
		.align	2
_uba_66:	.globl	_uba_66
		movl	$102,ubanum
		jmp	_uba
		.align	2
_uba_67:	.globl	_uba_67
		movl	$103,ubanum
		jmp	_uba
		.align	2
_uba_68:	.globl	_uba_68
		movl	$104,ubanum
		jmp	_uba
		.align	2
_uba_69:	.globl	_uba_69
		movl	$105,ubanum
		jmp	_uba
		.align	2
_uba_6a:	.globl	_uba_6a
		movl	$106,ubanum
		jmp	_uba
		.align	2
_uba_6b:	.globl	_uba_6b
		movl	$107,ubanum
		jmp	_uba
		.align	2
_uba_6c:	.globl	_uba_6c
		movl	$108,ubanum
		jmp	_uba
		.align	2
_uba_6d:	.globl	_uba_6d
		movl	$109,ubanum
		jmp	_uba
		.align	2
_uba_6e:	.globl	_uba_6e
		movl	$110,ubanum
		jmp	_uba
		.align	2
_uba_6f:	.globl	_uba_6f
		movl	$111,ubanum
		brb	_uba
		.align	2
_uba_70:	.globl	_uba_70
		movl	$112,ubanum
		brb	_uba
		.align	2
_uba_71:	.globl	_uba_71
		movl	$113,ubanum
		brb	_uba
		.align	2
_uba_72:	.globl	_uba_72
		movl	$114,ubanum
		brb	_uba
		.align	2
_uba_73:	.globl	_uba_73
		movl	$115,ubanum
		brb	_uba
		.align	2
_uba_74:	.globl	_uba_74
		movl	$116,ubanum
		brb	_uba
		.align	2
_uba_75:	.globl	_uba_75
		movl	$117,ubanum
		brb	_uba
		.align	2
_uba_76:	.globl	_uba_76
		movl	$118,ubanum
		brb	_uba
		.align	2
_uba_77:	.globl	_uba_77
		movl	$119,ubanum
		brb	_uba
		.align	2
_uba_78:	.globl	_uba_78
		movl	$120,ubanum
		brb	_uba
		.align	2
_uba_79:	.globl	_uba_79
		movl	$121,ubanum
		brb	_uba
		.align	2
_uba_7a:	.globl	_uba_7a
		movl	$122,ubanum
		brb	_uba
		.align	2
_uba_7b:	.globl	_uba_7b
		movl	$123,ubanum
		brb	_uba
		.align	2
_uba_7c:	.globl	_uba_7c
		movl	$124,ubanum
		brb	_uba
		.align	2
_uba_7d:	.globl	_uba_7d
		movl	$125,ubanum
		brb	_uba
		.align	2
_uba_7e:	.globl	_uba_7e
		movl	$126,ubanum
		brb	_uba
		.align	2
_uba_7f:	.globl	_uba_7f
		movl	$127,ubanum
_uba:		pushr	$0x7fff
		mfpr	$PR_IPL,r0
		pushl	r0
		pushl	ubanum
		calls	$2,_ubainterrupt
		popr	$0x7fff
		rei
#
# copyin(from, to, len) copies from userspace to kernelspace.
#

	.globl	_copyin
_copyin:.word	0x1c
	movl	4(ap),r0	# from
	movl	8(ap),r1	# to
	movl	12(ap),r2	# len

	movl	r0,r4
	movl	r2,r3

3:	prober	$3,r3,(r4)	# Check access to all pages.
	beql	1f
	cmpl	r3,$NBPG
	bleq	2f
	subl2	$NBPG,r3
	addl2	$NBPG,r4
	brb	3b

2:      movb    (r0)+,(r1)+       # XXX Should be done in a faster way.
        decl    r2              
        bneq    2b
        clrl    r0
        ret

1:	movl	$EFAULT,r0	# Didnt work...
	ret

#
# copyout(from, to, len) in the same manner as copyin()
#

	.globl	_copyout
_copyout:.word   0x1c
        movl    4(ap),r0        # from
        movl    8(ap),r1        # to
        movl    12(ap),r2       # len

        movl    r1,r4
        movl    r2,r3

3:      probew  $3,r3,(r4)	# Check access to all pages.
        beql    1b
        cmpl    r3,$NBPG
        bleq    2f
        subl2   $NBPG,r3
        addl2   $NBPG,r4
        brb     3b

2:	movb	(r0)+,(r1)+	# XXX Should be done in a faster way.
	decl	r2
	bneq	2b
	clrl	r0
        ret

#
# copystr(from, to, maxlen, *copied)
# Only used in kernel mode, doesnt check accessability.
#

	.globl	_copystr
_copystr:	.word 0x7c
        movl    4(ap),r4        # from
        movl    8(ap),r5        # to
        movl    12(ap),r2       # len
	movl	16(ap),r3	# copied
#	halt

	locc	$0, r2, (r4)	# check for null byte
	beql	1f

	subl3	r0, r2, r6	# Len to copy.
	incl	r6
	movl	r6,(r3)
	pushl	r6
	pushl	r5
	pushl	r4
	calls	$3,_bcopy
	movl	$0,r0
	ret

1:	pushl	r2
	pushl	r5
	pushl	r4
	calls   $3,_bcopy
	movl	$ENAMETOOLONG, r0
	ret



_kern_delay:	.globl _kern_delay
		.word	0x01
		mcoml	4(ap),r0		# Complement the delay len.
		mtpr	r0,$PR_NICR		# Set count register.
		mtpr 	$0x00000011,$PR_ICCS	# Set clock control.
1:		mfpr	$PR_ICCS,r0
		bicl2	$0xffffff7f,r0
		beql	1b
		ret

		
_loswtch:	.globl	_loswtch
#		halt
		mtpr	_curpcb,$PR_PCBB
		svpctx
		mtpr	_nypcb,$PR_PCBB
		ldpctx
#		halt
		rei


		.globl _savectx
_savectx:
		clrl	r0
		mfpr	$PR_P0BR,p0br
		mfpr    $PR_P1BR,p1br
		svpctx
		ldpctx
		mtpr	p0br,$PR_P0BR
		mtpr    p1br,$PR_P1BR
		mfpr	$PR_ESP,r0	# Child r0 != 0
		movl	_ustat,(r0)
		addl2	_uofset,68(r0)
		movl	_ustat,r0
		movl	(sp),(r0)
		movl	4(sp),4(r0)
#		halt
		rei


_icode:		.globl	_icode
	nop
	nop
	pushl   $0

        movl    $binit,r0
        subl2	$_icode,r0
        pushl   r0

	movl    $init,r0
	subl2	$_icode,r0
	pushl   r0
	movl	sp,ap
	subl2	$4,ap
        chmk	$SYS_execve
	pushl	r0
	chmk	$SYS_exit

init:   .asciz  "/sbin/init"

ainit:	.asciz	"init"
	.long	0
binit:	.long	ainit-_icode
	.long   0
	
eicode:

        .globl  _szicode
_szicode:
        .long   eicode-_icode



		.globl  _kstack
		.set    _kstack,USRSTACK

	
	.data
mbanum:		.long 0
ubanum:		.long 0
_bootdev:	.long 0
_boothowto:	.long 0
	.text
	
msg_stack:	.asciz  "Stack=0x%x\n"
msg_dot:	.asciz  "."
msg_nl:		.asciz	"\n\r"
msg_tillmain:	.asciz  "Returned to locore.\n\r"
msg_exit_init:  .asciz  "PANIC: kernel main() exited unexpectedly.\n\r"
msg_ipanic:	.asciz	"(dont) P A N I C.\n\r"


/*** DATA ********************************************************************/

	.data

		.globl _nextest
		.globl _memtest
		.globl _cpu_type
		.globl _ptab_len
		.globl _ptab_addr

p0br:		.long	0
p1br:		.long	0

p_text:		.long 0 ; .globl p_text		/* Start of text 	    */
l_text:		.long 0	; .globl l_text		/* Size of text 	    */
p_data:		.long 0 ; .globl p_data		/* Start of data 	    */
l_data:		.long 0	; .globl l_data		/* Size of data 	    */

p_prot1:	.long 0 ; .globl p_prot1	/* Protected page to detect 
							stack overflow  */
l_prot1:	.long 0 ; .globl l_prot1	/* NBPG bytes 		    */
p_stack:	.long 0 ; .globl p_stack	/* Stack area      	    */
l_stack:	.long 0 ; .globl l_stack	/* Size of kernel stack	    */
p_prot2:	.long 0 ; .globl p_prot2	/* Protected page to detect 
							stack underflow */
l_prot2:	.long 0 ; .globl l_prot2	/* NBPG bytes 		    */

p_prot3:	.long 0 ; .globl p_prot3	/* Protected page to detect 
							stack underflow */
l_prot3:	.long 0 ; .globl l_prot3	/* NBPG bytes 		    */

_proc0paddr:	.long 0 ; .globl _proc0paddr	/* Process 0 pointer */

p_cmap:		.long 0	; .globl p_cmap		/* Start of cmap	    */
v_cmap:		.long 0	; .globl v_cmap		/* Start of cmap	    */
l_cmap:		.long 0	; .globl l_cmap		/* Size of cmap		    */

p_ptab:		.long 0 ; .globl p_ptab		/* Start of page table 	    */
l_ptab:		.long 0 ; .globl l_ptab		/* Size of page table 	    */

_pte_cmap:	.long 0 ; .globl _pte_cmap	/* Address of PTE 
						   corresponding to cmap    */

_nextest:					# Test Nexus from C.
_memtest:	.long 0 ; .globl _memtest	# Memory test in progress.
_cpu_type:	.long 0                         # SID register contents.
_ptab_addr:	.long 0
_ptab_len:	.long 0

_phys_avail:	.long 0 ; .globl _phys_avail

mem_tab:	.long 0				# Mem table; VAX has contig. 
						#	memory, so only
		.long 0				# one start/end entry needed.
		.long 0
		.long 0

		.globl _nexus_conn
_nexus_conn:	.long 0,0,0,0,0,0,0,0		# Max 16 nexus. (At least 
						#	on the 750 ;)
		.long 0,0,0,0,0,0,0,0
