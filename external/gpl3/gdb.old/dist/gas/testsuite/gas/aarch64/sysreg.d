#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0+ <.*>:
   0:	d51b9c67 	msr	pmovsclr_el0, x7
   4:	d53b9c60 	mrs	x0, pmovsclr_el0
   8:	d51b9e67 	msr	pmovsset_el0, x7
   c:	d53b9e60 	mrs	x0, pmovsset_el0
  10:	d5380140 	mrs	x0, id_dfr0_el1
  14:	d5380100 	mrs	x0, id_pfr0_el1
  18:	d5380120 	mrs	x0, id_pfr1_el1
  1c:	d5380160 	mrs	x0, id_afr0_el1
  20:	d5380180 	mrs	x0, id_mmfr0_el1
  24:	d53801a0 	mrs	x0, id_mmfr1_el1
  28:	d53801c0 	mrs	x0, id_mmfr2_el1
  2c:	d53801e0 	mrs	x0, id_mmfr3_el1
  30:	d53802c0 	mrs	x0, id_mmfr4_el1
  34:	d5380200 	mrs	x0, id_isar0_el1
  38:	d5380220 	mrs	x0, id_isar1_el1
  3c:	d5380240 	mrs	x0, id_isar2_el1
  40:	d5380260 	mrs	x0, id_isar3_el1
  44:	d5380280 	mrs	x0, id_isar4_el1
  48:	d53802a0 	mrs	x0, id_isar5_el1
  4c:	d538cc00 	mrs	x0, s3_0_c12_c12_0
  50:	d5384600 	mrs	x0, s3_0_c4_c6_0
  54:	d5184600 	msr	s3_0_c4_c6_0, x0
  58:	d5310300 	mrs	x0, s2_1_c0_c3_0
  5c:	d5110300 	msr	s2_1_c0_c3_0, x0
