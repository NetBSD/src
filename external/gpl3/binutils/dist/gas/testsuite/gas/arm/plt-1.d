# name: Thumb branch to PLT
# as:
# objdump: -dr
# This test is only valid on ELF based ports.
#not-target: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd *-*-riscix*

.*: +file format .*arm.*


Disassembly of section \.text:

0+000 <Strong1>:
   0:	f7ff bffe 	b\.w	12 <Strong2>
			0: R_ARM_THM_JUMP24	Strong2
   4:	f7ff bffe 	b\.w	12 <Strong2>
			4: R_ARM_THM_JUMP24	Strong2
   8:	e7fe      	b\.n	12 <Strong2>
			8: R_ARM_THM_JUMP11	Strong2
   a:	e7fe      	b\.n	12 <Strong2>
			a: R_ARM_THM_JUMP11	Strong2
   c:	f7ff bffe 	b\.w	12 <Strong2>
			c: R_ARM_THM_JUMP24	Strong2
  10:	e7fe      	b\.n	12 <Strong2>
			10: R_ARM_THM_JUMP11	Strong2

0+012 <Strong2>:
  12:	f7ff bffe 	b\.w	0 <Strong1>
			12: R_ARM_THM_JUMP24	Strong1
  16:	f7ff bffe 	b\.w	0 <Strong1>
			16: R_ARM_THM_JUMP24	Strong1
  1a:	e7fe      	b\.n	0 <Strong1>
			1a: R_ARM_THM_JUMP11	Strong1
  1c:	e7fe      	b\.n	0 <Strong1>
			1c: R_ARM_THM_JUMP11	Strong1
  1e:	f7ff bffe 	b\.w	0 <Strong1>
			1e: R_ARM_THM_JUMP24	Strong1
  22:	e7fe      	b\.n	0 <Strong1>
			22: R_ARM_THM_JUMP11	Strong1
