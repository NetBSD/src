# name: Thumb branch to weak
# as:
# objdump: -dr
# This test is only valid on ELF based ports.
#not-target: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd *-*-riscix*

.*: +file format .*arm.*


Disassembly of section .text:

0+000 <Weak>:
   0:	e7fe      	b.n	2 <Strong>
			0: R_ARM_THM_JUMP11	Strong

0+002 <Strong>:
   2:	f7ff bffe 	b.w	0 <Random>
			2: R_ARM_THM_JUMP24	Random
   6:	f7ff bffe 	b.w	0 <Weak>
			6: R_ARM_THM_JUMP24	Weak
