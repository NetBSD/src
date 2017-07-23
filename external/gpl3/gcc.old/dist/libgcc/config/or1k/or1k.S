#include "or1k-asm.h"
/*
 * Assembly functions for software multiplication and devision.
 */

#define ENTRY(symbol)	\
	.align 4	;\
	.global symbol	;\
	.type	symbol, @function ;\
symbol:

#ifdef L__mulsi3
ENTRY(__mulsi3)
	l.addi r11,r0,0x0
	l.sfne r3,r11
OR1K_DELAYED(
	OR1K_INST(l.ori r5,r3,0x0),
	OR1K_INST(l.bnf 3f)
)
	l.addi r6,r0,0x0
1:
	l.andi r3,r5,0x1
	l.sfeq r3,r6
OR1K_DELAYED(
	OR1K_INST(l.srli r5,r5,0x1),
	OR1K_INST(l.bf 2f)
)
	l.add r11,r11,r4
2:
	l.sfne r5,r6
OR1K_DELAYED(
	OR1K_INST(l.slli r4,r4,0x1),
	OR1K_INST(l.bf 1b)
)
3:
OR1K_DELAYED_NOP(
	OR1K_INST(l.jr r9)
)
.size __mulsi3,.-__mulsi3
#endif

#ifdef L__udivsi3
.global __udivsi3_internal
.hidden __udivsi3_internal
__udivsi3_internal:
ENTRY(__udivsi3)
	l.addi          r1,r1,-4
        l.sw            0(r1),r9
        l.addi          r11,r0,0
        l.addi          r8,r4,0
        l.addi          r5,r3,0
        l.sfne          r8,r11
OR1K_DELAYED(
	OR1K_INST(l.addi r7,r0,0),
	OR1K_INST(l.bnf 4f)
)
	/* The following work equally on delay and no-delay implementations */
        l.sfgtu         r8,r5
        l.bf            5f
        l.sfeq          r8,r5
        l.bf            6f
        l.sfltu         r11,r8

OR1K_DELAYED(
	OR1K_INST(l.addi r13,r0,32),
        OR1K_INST(l.bnf 2f)
)
        l.movhi         r9,hi(0x80000000)
        l.addi          r6,r0,-1
1:
        l.and           r3,r5,r9
        l.slli          r4,r7,1
        l.addi          r15,r5,0
        l.srli          r3,r3,31
        l.add           r13,r13,r6
        l.or            r7,r4,r3
        l.sfltu         r7,r8
OR1K_DELAYED(
        OR1K_INST(l.slli r5,r5,1),
        OR1K_INST(l.bf 1b)
)
2:
        l.srli          r7,r7,1
        l.addi          r13,r13,1
        l.addi          r9,r0,0
        l.sfltu         r9,r13
OR1K_DELAYED(
        OR1K_INST(l.addi r5,r15,0),
        OR1K_INST(l.bnf 4f)
)
        l.movhi         r15,hi(0x80000000)
        l.addi          r17,r0,0
3:
        l.and           r3,r5,r15
        l.slli          r4,r7,1
        l.srli          r3,r3,31
        l.or            r7,r4,r3
        l.sub           r6,r7,r8
        l.and           r3,r6,r15
        l.srli          r3,r3,31
        l.addi          r4,r0,0
        l.sfne          r3,r4
OR1K_DELAYED(
        OR1K_INST(l.slli r3,r11,1),
        OR1K_INST(l.bf 1f)
)
        l.addi          r4,r0,1
1:
        l.slli          r5,r5,1
        l.sfne          r4,r17
OR1K_DELAYED(
        OR1K_INST(l.or r11,r3,r4),
        OR1K_INST(l.bnf 2f)
)
        l.addi          r7,r6,0
2:
        l.addi          r9,r9,1
        l.sfltu         r9,r13
OR1K_DELAYED_NOP(
        OR1K_INST(l.bf 3b)
)
OR1K_DELAYED_NOP(
	OR1K_INST(l.j 4f)
)
6:
OR1K_DELAYED(
	OR1K_INST(l.addi r11,r0,1),
	OR1K_INST(l.j 4f)
)
5:
	l.addi		r7,r5,0
4:
        l.lwz           r9,0(r1)
OR1K_DELAYED(
        OR1K_INST(l.addi r1,r1,4),
        OR1K_INST(l.jr r9)
)
.size __udivsi3,.-__udivsi3
#endif


#ifdef L__divsi3
ENTRY(__divsi3)
	l.addi          r1,r1,-8
        l.sw            0(r1),r9
        l.sw            4(r1),r14
        l.addi          r5,r3,0
        l.addi          r14,r0,0
        l.sflts         r5,r0
OR1K_DELAYED(
        OR1K_INST(l.addi r3,r0,0),
        OR1K_INST(l.bnf 1f)
)
        l.addi          r14,r0,1
        l.sub           r5,r0,r5
1:
        l.sflts         r4,r0
OR1K_DELAYED_NOP(
        OR1K_INST(l.bnf 1f)
)
        l.addi          r14,r14,1
        l.sub           r4,r0,r4
1:
OR1K_DELAYED(
        OR1K_INST(l.addi r3,r5,0),
        OR1K_INST(l.jal __udivsi3_internal)
)
        l.sfeqi         r14,1
OR1K_DELAYED_NOP(
        OR1K_INST(l.bnf 1f)
)
        l.sub           r11,r0,r11
1:
        l.lwz           r9,0(r1)
        l.lwz           r14,4(r1)
OR1K_DELAYED(
        OR1K_INST(l.addi r1,r1,8),
        OR1K_INST(l.jr r9)
)
.size __divsi3,.-__divsi3
#endif


#ifdef L__umodsi3
ENTRY(__umodsi3)
	l.addi          r1,r1,-4
	l.sw            0(r1),r9
OR1K_DELAYED_NOP(
	OR1K_INST(l.jal __udivsi3_internal)
)
	l.addi		r11,r7,0
	l.lwz           r9,0(r1)
OR1K_DELAYED(
	OR1K_INST(l.addi r1,r1,4),
	OR1K_INST(l.jr r9)
)
.size __umodsi3,.-__umodsi3
#endif


#ifdef L__modsi3
ENTRY(__modsi3)
        l.addi          r1,r1,-8
        l.sw            0(r1),r9
        l.sw            4(r1),r14
        l.addi          r14,r0,0
        l.sflts         r3,r0
OR1K_DELAYED_NOP(
        OR1K_INST(l.bnf 1f)
)
        l.addi          r14,r0,1
        l.sub           r3,r0,r3
1:
        l.sflts         r4,r0
OR1K_DELAYED_NOP(
        OR1K_INST(l.bnf 1f)
)
        l.sub           r4,r0,r4
1:
OR1K_DELAYED_NOP(
        OR1K_INST(l.jal __udivsi3_internal)
)
        l.sfeqi         r14,1
OR1K_DELAYED(
        OR1K_INST(l.addi r11,r7,0),
        OR1K_INST(l.bnf 1f)
)
        l.sub           r11,r0,r11
1:
        l.lwz           r9,0(r1)
        l.lwz           r14,4(r1)
OR1K_DELAYED(
        OR1K_INST(l.addi r1,r1,8),
        OR1K_INST(l.jr r9)
)
.size __modsi3,.-__modsi3
#endif
