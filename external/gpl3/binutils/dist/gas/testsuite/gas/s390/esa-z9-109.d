#name: s390 opcode
#objdump: -drw

.*: +file format .*

Disassembly of section .text:

.* <foo>:
.*:	b9 93 f0 68 [	 ]*troo	%r6,%r8,15
.*:	b9 92 f0 68 [	 ]*trot	%r6,%r8,15
.*:	b9 91 f0 68 [	 ]*trto	%r6,%r8,15
.*:	b9 90 f0 68 [	 ]*trtt	%r6,%r8,15
.*:	b2 2b 00 69 [	 ]*sske	%r6,%r9
