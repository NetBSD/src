#source: sve-movprfx_13.s
#warning_output: sve-movprfx_13.l
#as: -march=armv8-a+sve -I$srcdir/$subdir --generate-missing-build-notes=no
#objdump: -Dr -M notes

.* file format .*

Disassembly of section .*:

0+ <.*>:
[^:]+:	2598e3e0 	ptrue	p0.s
[^:]+:	0420bc01 	movprfx	z1, z0

0+8 <.*>:
[^:]+:	0497a021 	neg	z1.s, p0/m, z1.s  // note: output register of preceding `movprfx' used as input at operand 3
[^:]+:	d65f03c0 	ret

