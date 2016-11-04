#source: main.s
#source: gcdfn.s
#source: hidfn.s
#ld: --gc-sections --shared --version-script hideall.ld
#objdump: -dRT
# This test is only valid on ELF based ports.
# not-target: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd *-*-riscix*

# See PR ld/13990: a forced-local PLT reference to a
# forced-local symbol is GC'ed, trigging a BFD_ASSERT.

.*:     file format elf32-.*

DYNAMIC SYMBOL TABLE:
0+[0-9a-f]+ l    d  .text	0+              .text
0+ g    DO \*ABS\*	0+  NS          NS

Disassembly of section .text:

0+[0-9a-f]+ <_start>:
\s*[0-9a-f]+:\s+e52de004\s+push	{lr}		; \(str lr, \[sp, #-4\]!\)
\s*[0-9a-f]+:\s+eb000000\s+bl	[0-9a-f]+ <hidfn>
\s*[0-9a-f]+:\s+e8bd8000\s+ldmfd	sp!, {pc}

0+[0-9a-f]+ <hidfn>:
\s*[0-9a-f]+:\s+e8bd8000\s+ldmfd	sp!, {pc}
