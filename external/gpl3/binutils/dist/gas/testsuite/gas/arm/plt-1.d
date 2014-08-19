# name: Thumb branch to PLT
# as:
# objdump: -dr
# This test is only valid on ELF based ports.
#not-target: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd *-*-riscix*

.*: +file format .*arm.*


Disassembly of section \.text:

0+000 <Strong1>:
   0:	f7ff bffe 	b\.w	14 <Strong2>
			0: R_ARM_THM_JUMP24	Strong2
   4:	f7ff bffe 	b\.w	14 <Strong2>
			4: R_ARM_THM_JUMP24	Strong2
   8:	e7fe      	b\.n	14 <Strong2>
			8: R_ARM_THM_JUMP11	Strong2
   a:	f7ff bffe 	b\.w	14 <Strong2>
			a: R_ARM_THM_JUMP24	Strong2
   e:	f7ff bffe 	b\.w	14 <Strong2>
			e: R_ARM_THM_JUMP24	Strong2
  12:	e7fe      	b\.n	14 <Strong2>
			12: R_ARM_THM_JUMP11	Strong2

0+014 <Strong2>:
  14:	f7ff bffe 	b\.w	0 <Strong1>
			14: R_ARM_THM_JUMP24	Strong1
  18:	f7ff bffe 	b\.w	0 <Strong1>
			18: R_ARM_THM_JUMP24	Strong1
  1c:	e7fe      	b\.n	0 <Strong1>
			1c: R_ARM_THM_JUMP11	Strong1
  1e:	f7ff bffe 	b\.w	0 <Strong1>
			1e: R_ARM_THM_JUMP24	Strong1
  22:	f7ff bffe 	b\.w	0 <Strong1>
			22: R_ARM_THM_JUMP24	Strong1
  26:	e7fe      	b\.n	0 <Strong1>
			26: R_ARM_THM_JUMP11	Strong1
