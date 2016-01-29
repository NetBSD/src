
.*:     file format elf32-littlearm.*


Disassembly of section \.text:

00008000 <__stm32l4xx_veneer_0>:
    8000:	e899 01fe 	ldmia\.w	r9, {r1, r2, r3, r4, r5, r6, r7, r8}
    8004:	f000 b84a 	b\.w	809c <__stm32l4xx_veneer_0_r>
    8008:	f7f0 a000 	udf\.w	#0
    800c:	f7f0 a000 	udf\.w	#0

00008010 <__stm32l4xx_veneer_1>:
    8010:	e8b9 01fe 	ldmia\.w	r9!, {r1, r2, r3, r4, r5, r6, r7, r8}
    8014:	f000 b844 	b\.w	80a0 <__stm32l4xx_veneer_1_r>
    8018:	f7f0 a000 	udf\.w	#0
    801c:	f7f0 a000 	udf\.w	#0

00008020 <__stm32l4xx_veneer_2>:
    8020:	e919 01fe 	ldmdb	r9, {r1, r2, r3, r4, r5, r6, r7, r8}
    8024:	f000 b83e 	b\.w	80a4 <__stm32l4xx_veneer_2_r>
    8028:	f7f0 a000 	udf\.w	#0
    802c:	f7f0 a000 	udf\.w	#0

00008030 <__stm32l4xx_veneer_3>:
    8030:	e939 01fe 	ldmdb	r9!, {r1, r2, r3, r4, r5, r6, r7, r8}
    8034:	f000 b838 	b\.w	80a8 <__stm32l4xx_veneer_3_r>
    8038:	f7f0 a000 	udf\.w	#0
    803c:	f7f0 a000 	udf\.w	#0

00008040 <__stm32l4xx_veneer_4>:
    8040:	e8bd 01fe 	ldmia\.w	sp!, {r1, r2, r3, r4, r5, r6, r7, r8}
    8044:	f000 b832 	b\.w	80ac <__stm32l4xx_veneer_4_r>
    8048:	f7f0 a000 	udf\.w	#0
    804c:	f7f0 a000 	udf\.w	#0

00008050 <__stm32l4xx_veneer_5>:
    8050:	ecd9 0a08 	vldmia	r9, {s1-s8}
    8054:	f000 b82c 	b\.w	80b0 <__stm32l4xx_veneer_5_r>
    8058:	f7f0 a000 	udf\.w	#0
    805c:	f7f0 a000 	udf\.w	#0
    8060:	f7f0 a000 	udf\.w	#0
    8064:	f7f0 a000 	udf\.w	#0

00008068 <__stm32l4xx_veneer_6>:
    8068:	ecf6 4a08 	vldmia	r6!, {s9-s16}
    806c:	f000 b822 	b\.w	80b4 <__stm32l4xx_veneer_6_r>
    8070:	f7f0 a000 	udf\.w	#0
    8074:	f7f0 a000 	udf\.w	#0
    8078:	f7f0 a000 	udf\.w	#0
    807c:	f7f0 a000 	udf\.w	#0

00008080 <__stm32l4xx_veneer_7>:
    8080:	ecfd 0a08 	vpop	{s1-s8}
    8084:	f000 b818 	b\.w	80b8 <__stm32l4xx_veneer_7_r>
    8088:	f7f0 a000 	udf\.w	#0
    808c:	f7f0 a000 	udf\.w	#0
    8090:	f7f0 a000 	udf\.w	#0
    8094:	f7f0 a000 	udf\.w	#0

00008098 <_start>:
    8098:	f7ff bfb2 	b\.w	8000 <__stm32l4xx_veneer_0>

0000809c <__stm32l4xx_veneer_0_r>:
    809c:	f7ff bfb8 	b\.w	8010 <__stm32l4xx_veneer_1>

000080a0 <__stm32l4xx_veneer_1_r>:
    80a0:	f7ff bfbe 	b\.w	8020 <__stm32l4xx_veneer_2>

000080a4 <__stm32l4xx_veneer_2_r>:
    80a4:	f7ff bfc4 	b\.w	8030 <__stm32l4xx_veneer_3>

000080a8 <__stm32l4xx_veneer_3_r>:
    80a8:	f7ff bfca 	b\.w	8040 <__stm32l4xx_veneer_4>

000080ac <__stm32l4xx_veneer_4_r>:
    80ac:	f7ff bfd0 	b\.w	8050 <__stm32l4xx_veneer_5>

000080b0 <__stm32l4xx_veneer_5_r>:
    80b0:	f7ff bfda 	b\.w	8068 <__stm32l4xx_veneer_6>

000080b4 <__stm32l4xx_veneer_6_r>:
    80b4:	f7ff bfe4 	b\.w	8080 <__stm32l4xx_veneer_7>
