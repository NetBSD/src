#source: tlsld32.s
#as: -a32
#ld: -melf32ppc
#objdump: -dr
#target: powerpc*-*-*

.*:     file format .*

Disassembly of section \.text:

.*:
.*	nop
.*	addis   r29,r2,0
.*	mr      r3,r29
.*	addi    r3,r3,4096
.*	addis   r3,r3,0
.*	lwz     r3,-32768\(r3\)
.*	nop
.*	nop
.*	addis   r29,r2,0
.*	mr      r3,r29
.*	addi    r3,r3,4096
.*	lwz     r3,-32768\(r3\)
.*	nop
.*	nop
.*	nop
.*	nop
.*	nop
.*	addis   r29,r2,0
.*	mr      r3,r29
.*	addi    r3,r3,-28672
.*	lwz     r3,0\(r3\)
.*	nop
.*	nop
.*	nop
.*	addis   r29,r2,0
.*	mr      r3,r29
.*	addi    r3,r3,-28672
.*	lwz     r3,0\(r3\)
.*	nop
.*	nop
.*	nop
.*	nop
#pass
