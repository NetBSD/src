#target: arm*-*-*eabi
#source: main.s
#source: gcdfn.s
#source: hidfn.s
#ld: --gc-sections --shared --version-script hideall.ld
#objdump: -dRT

# See PR ld/13990: a forced-local PLT reference to a
# forced-local symbol is GC'ed, trigging a BFD_ASSERT.

.*:     file format elf32-.*

DYNAMIC SYMBOL TABLE:
0+124 l    d  .text	0+              .text
0+ g    DO \*ABS\*	0+  NS          NS

Disassembly of section .text:

0+124 <_start>:
 124:	e52de004 	push	{lr}		; \(str lr, \[sp, #-4\]!\)
 128:	eb000000 	bl	130 <hidfn>
 12c:	e8bd8000 	ldmfd	sp!, {pc}

0+130 <hidfn>:
 130:	e8bd8000 	ldmfd	sp!, {pc}
