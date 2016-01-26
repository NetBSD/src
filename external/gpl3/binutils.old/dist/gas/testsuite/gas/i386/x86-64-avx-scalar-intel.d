#as: -mavxscalar=256
#objdump: -dwMintel
#name: x86-64 AVX scalar insns (Intel disassembly)
#source: x86-64-avx-scalar.s

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
[ 	]*[a-f0-9]+:	c5 fd 2f f4          	vcomisd xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 fd 2f 21          	vcomisd xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fd 2e f4          	vucomisd xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 fd 2e 21          	vucomisd xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ff 10 21          	vmovsd xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ff 11 21          	vmovsd QWORD PTR \[rcx\],xmm4
[ 	]*[a-f0-9]+:	c4 e1 fd 7e e1       	vmovq  rcx,xmm4
[ 	]*[a-f0-9]+:	c4 e1 fd 6e e1       	vmovq  xmm4,rcx
[ 	]*[a-f0-9]+:	c4 e1 fd 7e e1       	vmovq  rcx,xmm4
[ 	]*[a-f0-9]+:	c4 e1 fd 6e e1       	vmovq  xmm4,rcx
[ 	]*[a-f0-9]+:	c5 fd d6 21          	vmovq  QWORD PTR \[rcx\],xmm4
[ 	]*[a-f0-9]+:	c5 fe 7e 21          	vmovq  xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ff 2d cc          	vcvtsd2si ecx,xmm4
[ 	]*[a-f0-9]+:	c5 ff 2d 09          	vcvtsd2si ecx,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ff 2c cc          	vcvttsd2si ecx,xmm4
[ 	]*[a-f0-9]+:	c5 ff 2c 09          	vcvttsd2si ecx,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 ff 2d cc       	vcvtsd2si rcx,xmm4
[ 	]*[a-f0-9]+:	c4 e1 ff 2d 09       	vcvtsd2si rcx,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 ff 2c cc       	vcvttsd2si rcx,xmm4
[ 	]*[a-f0-9]+:	c4 e1 ff 2c 09       	vcvttsd2si rcx,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 df 2a f1       	vcvtsi2sd xmm6,xmm4,rcx
[ 	]*[a-f0-9]+:	c4 e1 df 2a 31       	vcvtsi2sd xmm6,xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 de 2a f1       	vcvtsi2ss xmm6,xmm4,rcx
[ 	]*[a-f0-9]+:	c4 e1 de 2a 31       	vcvtsi2ss xmm6,xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 07       	vcmpordsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 07       	vcmpordsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e3 4d 0b d4 07    	vroundsd xmm2,xmm6,xmm4,0x7
[ 	]*[a-f0-9]+:	c4 e3 4d 0b 11 07    	vroundsd xmm2,xmm6,QWORD PTR \[rcx\],0x7
[ 	]*[a-f0-9]+:	c5 cf 58 d4          	vaddsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 58 11          	vaddsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 5a d4          	vcvtsd2ss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 5a 11          	vcvtsd2ss xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 5e d4          	vdivsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 5e 11          	vdivsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 5f d4          	vmaxsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 5f 11          	vmaxsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 5d d4          	vminsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 5d 11          	vminsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 59 d4          	vmulsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 59 11          	vmulsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 51 d4          	vsqrtsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 51 11          	vsqrtsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 5c d4          	vsubsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 5c 11          	vsubsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 00       	vcmpeqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 00       	vcmpeqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 01       	vcmpltsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 01       	vcmpltsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 02       	vcmplesd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 02       	vcmplesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 03       	vcmpunordsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 03       	vcmpunordsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 04       	vcmpneqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 04       	vcmpneqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 05       	vcmpnltsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 05       	vcmpnltsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 06       	vcmpnlesd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 06       	vcmpnlesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 07       	vcmpordsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 07       	vcmpordsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 08       	vcmpeq_uqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 08       	vcmpeq_uqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 09       	vcmpngesd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 09       	vcmpngesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 0a       	vcmpngtsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 0a       	vcmpngtsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 0b       	vcmpfalsesd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 0b       	vcmpfalsesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 0c       	vcmpneq_oqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 0c       	vcmpneq_oqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 0d       	vcmpgesd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 0d       	vcmpgesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 0e       	vcmpgtsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 0e       	vcmpgtsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 0f       	vcmptruesd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 0f       	vcmptruesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 10       	vcmpeq_ossd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 10       	vcmpeq_ossd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 11       	vcmplt_oqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 11       	vcmplt_oqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 12       	vcmple_oqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 12       	vcmple_oqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 13       	vcmpunord_ssd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 13       	vcmpunord_ssd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 14       	vcmpneq_ussd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 14       	vcmpneq_ussd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 15       	vcmpnlt_uqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 15       	vcmpnlt_uqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 16       	vcmpnle_uqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 16       	vcmpnle_uqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 17       	vcmpord_ssd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 17       	vcmpord_ssd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 18       	vcmpeq_ussd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 18       	vcmpeq_ussd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 19       	vcmpnge_uqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 19       	vcmpnge_uqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 1a       	vcmpngt_uqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 1a       	vcmpngt_uqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 1b       	vcmpfalse_ossd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 1b       	vcmpfalse_ossd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 1c       	vcmpneq_ossd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 1c       	vcmpneq_ossd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 1d       	vcmpge_oqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 1d       	vcmpge_oqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 1e       	vcmpgt_oqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 1e       	vcmpgt_oqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 1f       	vcmptrue_ussd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 1f       	vcmptrue_ussd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 58 d4          	vaddss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 58 11          	vaddss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 5a d4          	vcvtss2sd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 5a 11          	vcvtss2sd xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 5e d4          	vdivss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 5e 11          	vdivss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 5f d4          	vmaxss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 5f 11          	vmaxss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 5d d4          	vminss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 5d 11          	vminss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 59 d4          	vmulss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 59 11          	vmulss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 53 d4          	vrcpss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 53 11          	vrcpss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 52 d4          	vrsqrtss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 52 11          	vrsqrtss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 51 d4          	vsqrtss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 51 11          	vsqrtss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 5c d4          	vsubss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 5c 11          	vsubss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 00       	vcmpeqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 00       	vcmpeqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 01       	vcmpltss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 01       	vcmpltss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 02       	vcmpless xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 02       	vcmpless xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 03       	vcmpunordss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 03       	vcmpunordss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 04       	vcmpneqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 04       	vcmpneqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 05       	vcmpnltss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 05       	vcmpnltss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 06       	vcmpnless xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 06       	vcmpnless xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 07       	vcmpordss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 07       	vcmpordss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 08       	vcmpeq_uqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 08       	vcmpeq_uqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 09       	vcmpngess xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 09       	vcmpngess xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 0a       	vcmpngtss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 0a       	vcmpngtss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 0b       	vcmpfalsess xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 0b       	vcmpfalsess xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 0c       	vcmpneq_oqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 0c       	vcmpneq_oqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 0d       	vcmpgess xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 0d       	vcmpgess xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 0e       	vcmpgtss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 0e       	vcmpgtss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 0f       	vcmptruess xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 0f       	vcmptruess xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 10       	vcmpeq_osss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 10       	vcmpeq_osss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 11       	vcmplt_oqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 11       	vcmplt_oqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 12       	vcmple_oqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 12       	vcmple_oqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 13       	vcmpunord_sss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 13       	vcmpunord_sss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 14       	vcmpneq_usss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 14       	vcmpneq_usss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 15       	vcmpnlt_uqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 15       	vcmpnlt_uqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 16       	vcmpnle_uqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 16       	vcmpnle_uqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 17       	vcmpord_sss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 17       	vcmpord_sss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 18       	vcmpeq_usss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 18       	vcmpeq_usss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 19       	vcmpnge_uqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 19       	vcmpnge_uqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 1a       	vcmpngt_uqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 1a       	vcmpngt_uqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 1b       	vcmpfalse_osss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 1b       	vcmpfalse_osss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 1c       	vcmpneq_osss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 1c       	vcmpneq_osss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 1d       	vcmpge_oqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 1d       	vcmpge_oqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 1e       	vcmpgt_oqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 1e       	vcmpgt_oqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 1f       	vcmptrue_usss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 1f       	vcmptrue_usss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fc 2f f4          	vcomiss xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 fc 2f 21          	vcomiss xmm4,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fc 2e f4          	vucomiss xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 fc 2e 21          	vucomiss xmm4,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fe 10 21          	vmovss xmm4,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fe 11 21          	vmovss DWORD PTR \[rcx\],xmm4
[ 	]*[a-f0-9]+:	c5 fd 7e e1          	vmovd  ecx,xmm4
[ 	]*[a-f0-9]+:	c5 fd 7e 21          	vmovd  DWORD PTR \[rcx\],xmm4
[ 	]*[a-f0-9]+:	c5 fd 6e e1          	vmovd  xmm4,ecx
[ 	]*[a-f0-9]+:	c5 fd 6e 21          	vmovd  xmm4,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fe 2d cc          	vcvtss2si ecx,xmm4
[ 	]*[a-f0-9]+:	c5 fe 2d 09          	vcvtss2si ecx,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fe 2c cc          	vcvttss2si ecx,xmm4
[ 	]*[a-f0-9]+:	c5 fe 2c 09          	vcvttss2si ecx,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 fe 2d cc       	vcvtss2si rcx,xmm4
[ 	]*[a-f0-9]+:	c4 e1 fe 2d 09       	vcvtss2si rcx,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 fe 2c cc       	vcvttss2si rcx,xmm4
[ 	]*[a-f0-9]+:	c4 e1 fe 2c 09       	vcvttss2si rcx,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 df 2a f1          	vcvtsi2sd xmm6,xmm4,ecx
[ 	]*[a-f0-9]+:	c5 df 2a 31          	vcvtsi2sd xmm6,xmm4,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 de 2a f1          	vcvtsi2ss xmm6,xmm4,ecx
[ 	]*[a-f0-9]+:	c5 de 2a 31          	vcvtsi2ss xmm6,xmm4,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 07       	vcmpordss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 07       	vcmpordss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e3 4d 0a d4 07    	vroundss xmm2,xmm6,xmm4,0x7
[ 	]*[a-f0-9]+:	c4 e3 4d 0a 11 07    	vroundss xmm2,xmm6,DWORD PTR \[rcx\],0x7
[ 	]*[a-f0-9]+:	c5 fe 7e f4          	vmovq  xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 10 d4          	vmovsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 10 d4          	vmovss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 7d 7e 04 25 78 56 34 12 	vmovd  DWORD PTR ds:0x12345678,xmm8
[ 	]*[a-f0-9]+:	c5 3f 2a 3c 25 78 56 34 12 	vcvtsi2sd xmm15,xmm8,DWORD PTR ds:0x12345678
[ 	]*[a-f0-9]+:	c5 7d 7e 45 00       	vmovd  DWORD PTR \[rbp\+0x0\],xmm8
[ 	]*[a-f0-9]+:	c5 3f 2a 7d 00       	vcvtsi2sd xmm15,xmm8,DWORD PTR \[rbp\+0x0\]
[ 	]*[a-f0-9]+:	c5 7d 7e 04 24       	vmovd  DWORD PTR \[rsp\],xmm8
[ 	]*[a-f0-9]+:	c5 3f 2a 3c 24       	vcvtsi2sd xmm15,xmm8,DWORD PTR \[rsp\]
[ 	]*[a-f0-9]+:	c5 7d 7e 85 99 00 00 00 	vmovd  DWORD PTR \[rbp\+0x99\],xmm8
[ 	]*[a-f0-9]+:	c5 3f 2a bd 99 00 00 00 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[rbp\+0x99\]
[ 	]*[a-f0-9]+:	c4 41 7d 7e 87 99 00 00 00 	vmovd  DWORD PTR \[r15\+0x99\],xmm8
[ 	]*[a-f0-9]+:	c4 41 3f 2a bf 99 00 00 00 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[r15\+0x99\]
[ 	]*[a-f0-9]+:	c5 7d 7e 05 99 00 00 00 	vmovd  DWORD PTR \[rip\+0x99\],xmm8        # 4f9 <_start\+0x4f9>
[ 	]*[a-f0-9]+:	c5 3f 2a 3d 99 00 00 00 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[rip\+0x99\]        # 501 <_start\+0x501>
[ 	]*[a-f0-9]+:	c5 7d 7e 84 24 99 00 00 00 	vmovd  DWORD PTR \[rsp\+0x99\],xmm8
[ 	]*[a-f0-9]+:	c5 3f 2a bc 24 99 00 00 00 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[rsp\+0x99\]
[ 	]*[a-f0-9]+:	c4 41 7d 7e 84 24 99 00 00 00 	vmovd  DWORD PTR \[r12\+0x99\],xmm8
[ 	]*[a-f0-9]+:	c4 41 3f 2a bc 24 99 00 00 00 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[r12\+0x99\]
[ 	]*[a-f0-9]+:	c5 7d 7e 04 25 67 ff ff ff 	vmovd  DWORD PTR ds:0xffffffffffffff67,xmm8
[ 	]*[a-f0-9]+:	c5 3f 2a 3c 25 67 ff ff ff 	vcvtsi2sd xmm15,xmm8,DWORD PTR ds:0xffffffffffffff67
[ 	]*[a-f0-9]+:	c5 7d 7e 04 65 67 ff ff ff 	vmovd  DWORD PTR \[riz\*2-0x99\],xmm8
[ 	]*[a-f0-9]+:	c5 3f 2a 3c 65 67 ff ff ff 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[riz\*2-0x99\]
[ 	]*[a-f0-9]+:	c5 7d 7e 84 23 67 ff ff ff 	vmovd  DWORD PTR \[rbx\+riz\*1-0x99\],xmm8
[ 	]*[a-f0-9]+:	c5 3f 2a bc 23 67 ff ff ff 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[rbx\+riz\*1-0x99\]
[ 	]*[a-f0-9]+:	c5 7d 7e 84 63 67 ff ff ff 	vmovd  DWORD PTR \[rbx\+riz\*2-0x99\],xmm8
[ 	]*[a-f0-9]+:	c5 3f 2a bc 63 67 ff ff ff 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[rbx\+riz\*2-0x99\]
[ 	]*[a-f0-9]+:	c4 01 7d 7e 84 bc 67 ff ff ff 	vmovd  DWORD PTR \[r12\+r15\*4-0x99\],xmm8
[ 	]*[a-f0-9]+:	c4 01 3f 2a bc bc 67 ff ff ff 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[r12\+r15\*4-0x99\]
[ 	]*[a-f0-9]+:	c4 01 7d 7e 84 f8 67 ff ff ff 	vmovd  DWORD PTR \[r8\+r15\*8-0x99\],xmm8
[ 	]*[a-f0-9]+:	c4 01 3f 2a bc f8 67 ff ff ff 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[r8\+r15\*8-0x99\]
[ 	]*[a-f0-9]+:	c4 21 7d 7e 84 ad 67 ff ff ff 	vmovd  DWORD PTR \[rbp\+r13\*4-0x99\],xmm8
[ 	]*[a-f0-9]+:	c4 21 3f 2a bc ad 67 ff ff ff 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[rbp\+r13\*4-0x99\]
[ 	]*[a-f0-9]+:	c4 21 7d 7e 84 24 67 ff ff ff 	vmovd  DWORD PTR \[rsp\+r12\*1-0x99\],xmm8
[ 	]*[a-f0-9]+:	c4 21 3f 2a bc 24 67 ff ff ff 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[rsp\+r12\*1-0x99\]
[ 	]*[a-f0-9]+:	c4 41 7d 7e c0       	vmovd  r8d,xmm8
[ 	]*[a-f0-9]+:	c4 41 7f 2d c0       	vcvtsd2si r8d,xmm8
[ 	]*[a-f0-9]+:	c4 41 3f 2a f8       	vcvtsi2sd xmm15,xmm8,r8d
[ 	]*[a-f0-9]+:	c4 61 ff 2d 01       	vcvtsd2si r8,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 61 fe 2d 01       	vcvtss2si r8,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fd 2f f4          	vcomisd xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 fd 2f 21          	vcomisd xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fd 2f 21          	vcomisd xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fd 2e f4          	vucomisd xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 fd 2e 21          	vucomisd xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fd 2e 21          	vucomisd xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ff 10 21          	vmovsd xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ff 10 21          	vmovsd xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ff 11 21          	vmovsd QWORD PTR \[rcx\],xmm4
[ 	]*[a-f0-9]+:	c5 ff 11 21          	vmovsd QWORD PTR \[rcx\],xmm4
[ 	]*[a-f0-9]+:	c4 e1 fd 7e e1       	vmovq  rcx,xmm4
[ 	]*[a-f0-9]+:	c4 e1 fd 6e e1       	vmovq  xmm4,rcx
[ 	]*[a-f0-9]+:	c5 fd 7e 21          	vmovd  DWORD PTR \[rcx\],xmm4
[ 	]*[a-f0-9]+:	c5 fd 6e 21          	vmovd  xmm4,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 fd 7e e1       	vmovq  rcx,xmm4
[ 	]*[a-f0-9]+:	c4 e1 fd 6e e1       	vmovq  xmm4,rcx
[ 	]*[a-f0-9]+:	c5 fd d6 21          	vmovq  QWORD PTR \[rcx\],xmm4
[ 	]*[a-f0-9]+:	c5 fe 7e 21          	vmovq  xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fd d6 21          	vmovq  QWORD PTR \[rcx\],xmm4
[ 	]*[a-f0-9]+:	c5 fe 7e 21          	vmovq  xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ff 2d cc          	vcvtsd2si ecx,xmm4
[ 	]*[a-f0-9]+:	c5 ff 2d 09          	vcvtsd2si ecx,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ff 2d 09          	vcvtsd2si ecx,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ff 2c cc          	vcvttsd2si ecx,xmm4
[ 	]*[a-f0-9]+:	c5 ff 2c 09          	vcvttsd2si ecx,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ff 2c 09          	vcvttsd2si ecx,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 ff 2d cc       	vcvtsd2si rcx,xmm4
[ 	]*[a-f0-9]+:	c4 e1 ff 2d 09       	vcvtsd2si rcx,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 ff 2d 09       	vcvtsd2si rcx,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 ff 2c cc       	vcvttsd2si rcx,xmm4
[ 	]*[a-f0-9]+:	c4 e1 ff 2c 09       	vcvttsd2si rcx,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 ff 2c 09       	vcvttsd2si rcx,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 df 2a f1       	vcvtsi2sd xmm6,xmm4,rcx
[ 	]*[a-f0-9]+:	c4 e1 df 2a 31       	vcvtsi2sd xmm6,xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 df 2a 31       	vcvtsi2sd xmm6,xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 de 2a f1       	vcvtsi2ss xmm6,xmm4,rcx
[ 	]*[a-f0-9]+:	c4 e1 de 2a 31       	vcvtsi2ss xmm6,xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 de 2a 31       	vcvtsi2ss xmm6,xmm4,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 07       	vcmpordsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 07       	vcmpordsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 07       	vcmpordsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e3 4d 0b d4 07    	vroundsd xmm2,xmm6,xmm4,0x7
[ 	]*[a-f0-9]+:	c4 e3 4d 0b 11 07    	vroundsd xmm2,xmm6,QWORD PTR \[rcx\],0x7
[ 	]*[a-f0-9]+:	c4 e3 4d 0b 11 07    	vroundsd xmm2,xmm6,QWORD PTR \[rcx\],0x7
[ 	]*[a-f0-9]+:	c5 cf 58 d4          	vaddsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 58 11          	vaddsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 58 11          	vaddsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 5a d4          	vcvtsd2ss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 5a 11          	vcvtsd2ss xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 5a 11          	vcvtsd2ss xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 5e d4          	vdivsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 5e 11          	vdivsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 5e 11          	vdivsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 5f d4          	vmaxsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 5f 11          	vmaxsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 5f 11          	vmaxsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 5d d4          	vminsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 5d 11          	vminsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 5d 11          	vminsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 59 d4          	vmulsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 59 11          	vmulsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 59 11          	vmulsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 51 d4          	vsqrtsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 51 11          	vsqrtsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 51 11          	vsqrtsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 5c d4          	vsubsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 5c 11          	vsubsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf 5c 11          	vsubsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 00       	vcmpeqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 00       	vcmpeqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 00       	vcmpeqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 01       	vcmpltsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 01       	vcmpltsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 01       	vcmpltsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 02       	vcmplesd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 02       	vcmplesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 02       	vcmplesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 03       	vcmpunordsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 03       	vcmpunordsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 03       	vcmpunordsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 04       	vcmpneqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 04       	vcmpneqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 04       	vcmpneqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 05       	vcmpnltsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 05       	vcmpnltsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 05       	vcmpnltsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 06       	vcmpnlesd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 06       	vcmpnlesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 06       	vcmpnlesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 07       	vcmpordsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 07       	vcmpordsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 07       	vcmpordsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 08       	vcmpeq_uqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 08       	vcmpeq_uqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 08       	vcmpeq_uqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 09       	vcmpngesd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 09       	vcmpngesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 09       	vcmpngesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 0a       	vcmpngtsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 0a       	vcmpngtsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 0a       	vcmpngtsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 0b       	vcmpfalsesd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 0b       	vcmpfalsesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 0b       	vcmpfalsesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 0c       	vcmpneq_oqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 0c       	vcmpneq_oqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 0c       	vcmpneq_oqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 0d       	vcmpgesd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 0d       	vcmpgesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 0d       	vcmpgesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 0e       	vcmpgtsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 0e       	vcmpgtsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 0e       	vcmpgtsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 0f       	vcmptruesd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 0f       	vcmptruesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 0f       	vcmptruesd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 10       	vcmpeq_ossd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 10       	vcmpeq_ossd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 10       	vcmpeq_ossd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 11       	vcmplt_oqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 11       	vcmplt_oqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 11       	vcmplt_oqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 12       	vcmple_oqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 12       	vcmple_oqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 12       	vcmple_oqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 13       	vcmpunord_ssd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 13       	vcmpunord_ssd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 13       	vcmpunord_ssd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 14       	vcmpneq_ussd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 14       	vcmpneq_ussd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 14       	vcmpneq_ussd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 15       	vcmpnlt_uqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 15       	vcmpnlt_uqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 15       	vcmpnlt_uqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 16       	vcmpnle_uqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 16       	vcmpnle_uqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 16       	vcmpnle_uqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 17       	vcmpord_ssd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 17       	vcmpord_ssd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 17       	vcmpord_ssd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 18       	vcmpeq_ussd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 18       	vcmpeq_ussd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 18       	vcmpeq_ussd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 19       	vcmpnge_uqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 19       	vcmpnge_uqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 19       	vcmpnge_uqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 1a       	vcmpngt_uqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 1a       	vcmpngt_uqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 1a       	vcmpngt_uqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 1b       	vcmpfalse_ossd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 1b       	vcmpfalse_ossd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 1b       	vcmpfalse_ossd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 1c       	vcmpneq_ossd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 1c       	vcmpneq_ossd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 1c       	vcmpneq_ossd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 1d       	vcmpge_oqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 1d       	vcmpge_oqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 1d       	vcmpge_oqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 1e       	vcmpgt_oqsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 1e       	vcmpgt_oqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 1e       	vcmpgt_oqsd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 d4 1f       	vcmptrue_ussd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf c2 11 1f       	vcmptrue_ussd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 cf c2 11 1f       	vcmptrue_ussd xmm2,xmm6,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 58 d4          	vaddss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 58 11          	vaddss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 58 11          	vaddss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 5a d4          	vcvtss2sd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 5a 11          	vcvtss2sd xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 5a 11          	vcvtss2sd xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 5e d4          	vdivss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 5e 11          	vdivss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 5e 11          	vdivss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 5f d4          	vmaxss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 5f 11          	vmaxss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 5f 11          	vmaxss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 5d d4          	vminss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 5d 11          	vminss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 5d 11          	vminss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 59 d4          	vmulss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 59 11          	vmulss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 59 11          	vmulss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 53 d4          	vrcpss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 53 11          	vrcpss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 53 11          	vrcpss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 52 d4          	vrsqrtss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 52 11          	vrsqrtss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 52 11          	vrsqrtss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 51 d4          	vsqrtss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 51 11          	vsqrtss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 51 11          	vsqrtss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 5c d4          	vsubss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 5c 11          	vsubss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce 5c 11          	vsubss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 00       	vcmpeqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 00       	vcmpeqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 00       	vcmpeqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 01       	vcmpltss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 01       	vcmpltss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 01       	vcmpltss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 02       	vcmpless xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 02       	vcmpless xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 02       	vcmpless xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 03       	vcmpunordss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 03       	vcmpunordss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 03       	vcmpunordss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 04       	vcmpneqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 04       	vcmpneqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 04       	vcmpneqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 05       	vcmpnltss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 05       	vcmpnltss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 05       	vcmpnltss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 06       	vcmpnless xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 06       	vcmpnless xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 06       	vcmpnless xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 07       	vcmpordss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 07       	vcmpordss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 07       	vcmpordss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 08       	vcmpeq_uqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 08       	vcmpeq_uqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 08       	vcmpeq_uqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 09       	vcmpngess xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 09       	vcmpngess xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 09       	vcmpngess xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 0a       	vcmpngtss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 0a       	vcmpngtss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 0a       	vcmpngtss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 0b       	vcmpfalsess xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 0b       	vcmpfalsess xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 0b       	vcmpfalsess xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 0c       	vcmpneq_oqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 0c       	vcmpneq_oqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 0c       	vcmpneq_oqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 0d       	vcmpgess xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 0d       	vcmpgess xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 0d       	vcmpgess xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 0e       	vcmpgtss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 0e       	vcmpgtss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 0e       	vcmpgtss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 0f       	vcmptruess xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 0f       	vcmptruess xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 0f       	vcmptruess xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 10       	vcmpeq_osss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 10       	vcmpeq_osss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 10       	vcmpeq_osss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 11       	vcmplt_oqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 11       	vcmplt_oqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 11       	vcmplt_oqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 12       	vcmple_oqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 12       	vcmple_oqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 12       	vcmple_oqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 13       	vcmpunord_sss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 13       	vcmpunord_sss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 13       	vcmpunord_sss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 14       	vcmpneq_usss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 14       	vcmpneq_usss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 14       	vcmpneq_usss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 15       	vcmpnlt_uqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 15       	vcmpnlt_uqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 15       	vcmpnlt_uqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 16       	vcmpnle_uqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 16       	vcmpnle_uqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 16       	vcmpnle_uqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 17       	vcmpord_sss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 17       	vcmpord_sss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 17       	vcmpord_sss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 18       	vcmpeq_usss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 18       	vcmpeq_usss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 18       	vcmpeq_usss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 19       	vcmpnge_uqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 19       	vcmpnge_uqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 19       	vcmpnge_uqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 1a       	vcmpngt_uqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 1a       	vcmpngt_uqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 1a       	vcmpngt_uqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 1b       	vcmpfalse_osss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 1b       	vcmpfalse_osss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 1b       	vcmpfalse_osss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 1c       	vcmpneq_osss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 1c       	vcmpneq_osss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 1c       	vcmpneq_osss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 1d       	vcmpge_oqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 1d       	vcmpge_oqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 1d       	vcmpge_oqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 1e       	vcmpgt_oqss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 1e       	vcmpgt_oqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 1e       	vcmpgt_oqss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 1f       	vcmptrue_usss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 1f       	vcmptrue_usss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 1f       	vcmptrue_usss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fc 2f f4          	vcomiss xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 fc 2f 21          	vcomiss xmm4,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fc 2f 21          	vcomiss xmm4,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fc 2e f4          	vucomiss xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 fc 2e 21          	vucomiss xmm4,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fc 2e 21          	vucomiss xmm4,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fe 10 21          	vmovss xmm4,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fe 10 21          	vmovss xmm4,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fe 11 21          	vmovss DWORD PTR \[rcx\],xmm4
[ 	]*[a-f0-9]+:	c5 fe 11 21          	vmovss DWORD PTR \[rcx\],xmm4
[ 	]*[a-f0-9]+:	c5 fd 7e e1          	vmovd  ecx,xmm4
[ 	]*[a-f0-9]+:	c5 fd 7e 21          	vmovd  DWORD PTR \[rcx\],xmm4
[ 	]*[a-f0-9]+:	c5 fd 6e e1          	vmovd  xmm4,ecx
[ 	]*[a-f0-9]+:	c5 fd 6e 21          	vmovd  xmm4,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fd 7e 21          	vmovd  DWORD PTR \[rcx\],xmm4
[ 	]*[a-f0-9]+:	c5 fd 6e 21          	vmovd  xmm4,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fe 2d cc          	vcvtss2si ecx,xmm4
[ 	]*[a-f0-9]+:	c5 fe 2d 09          	vcvtss2si ecx,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fe 2d 09          	vcvtss2si ecx,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fe 2c cc          	vcvttss2si ecx,xmm4
[ 	]*[a-f0-9]+:	c5 fe 2c 09          	vcvttss2si ecx,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 fe 2c 09          	vcvttss2si ecx,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 fe 2d cc       	vcvtss2si rcx,xmm4
[ 	]*[a-f0-9]+:	c4 e1 fe 2d 09       	vcvtss2si rcx,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 fe 2d 09       	vcvtss2si rcx,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 fe 2c cc       	vcvttss2si rcx,xmm4
[ 	]*[a-f0-9]+:	c4 e1 fe 2c 09       	vcvttss2si rcx,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e1 fe 2c 09       	vcvttss2si rcx,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 df 2a f1          	vcvtsi2sd xmm6,xmm4,ecx
[ 	]*[a-f0-9]+:	c5 df 2a 31          	vcvtsi2sd xmm6,xmm4,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 de 2a f1          	vcvtsi2ss xmm6,xmm4,ecx
[ 	]*[a-f0-9]+:	c5 de 2a 31          	vcvtsi2ss xmm6,xmm4,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 d4 07       	vcmpordss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce c2 11 07       	vcmpordss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c5 ce c2 11 07       	vcmpordss xmm2,xmm6,DWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 e3 4d 0a d4 07    	vroundss xmm2,xmm6,xmm4,0x7
[ 	]*[a-f0-9]+:	c4 e3 4d 0a 11 07    	vroundss xmm2,xmm6,DWORD PTR \[rcx\],0x7
[ 	]*[a-f0-9]+:	c4 e3 4d 0a 11 07    	vroundss xmm2,xmm6,DWORD PTR \[rcx\],0x7
[ 	]*[a-f0-9]+:	c5 fe 7e f4          	vmovq  xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 cf 10 d4          	vmovsd xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 ce 10 d4          	vmovss xmm2,xmm6,xmm4
[ 	]*[a-f0-9]+:	c5 7d 7e 04 25 78 56 34 12 	vmovd  DWORD PTR ds:0x12345678,xmm8
[ 	]*[a-f0-9]+:	c5 3f 2a 3c 25 78 56 34 12 	vcvtsi2sd xmm15,xmm8,DWORD PTR ds:0x12345678
[ 	]*[a-f0-9]+:	c5 7d 7e 45 00       	vmovd  DWORD PTR \[rbp\+0x0\],xmm8
[ 	]*[a-f0-9]+:	c5 3f 2a 7d 00       	vcvtsi2sd xmm15,xmm8,DWORD PTR \[rbp\+0x0\]
[ 	]*[a-f0-9]+:	c5 7d 7e 85 99 00 00 00 	vmovd  DWORD PTR \[rbp\+0x99\],xmm8
[ 	]*[a-f0-9]+:	c5 3f 2a bd 99 00 00 00 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[rbp\+0x99\]
[ 	]*[a-f0-9]+:	c4 41 7d 7e 87 99 00 00 00 	vmovd  DWORD PTR \[r15\+0x99\],xmm8
[ 	]*[a-f0-9]+:	c4 41 3f 2a bf 99 00 00 00 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[r15\+0x99\]
[ 	]*[a-f0-9]+:	c5 7d 7e 05 99 00 00 00 	vmovd  DWORD PTR \[rip\+0x99\],xmm8        # c32 <_start\+0xc32>
[ 	]*[a-f0-9]+:	c5 3f 2a 3d 99 00 00 00 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[rip\+0x99\]        # c3a <_start\+0xc3a>
[ 	]*[a-f0-9]+:	c5 7d 7e 84 24 99 00 00 00 	vmovd  DWORD PTR \[rsp\+0x99\],xmm8
[ 	]*[a-f0-9]+:	c5 3f 2a bc 24 99 00 00 00 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[rsp\+0x99\]
[ 	]*[a-f0-9]+:	c4 41 7d 7e 84 24 99 00 00 00 	vmovd  DWORD PTR \[r12\+0x99\],xmm8
[ 	]*[a-f0-9]+:	c4 41 3f 2a bc 24 99 00 00 00 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[r12\+0x99\]
[ 	]*[a-f0-9]+:	c5 7d 7e 04 25 67 ff ff ff 	vmovd  DWORD PTR ds:0xffffffffffffff67,xmm8
[ 	]*[a-f0-9]+:	c5 3f 2a 3c 25 67 ff ff ff 	vcvtsi2sd xmm15,xmm8,DWORD PTR ds:0xffffffffffffff67
[ 	]*[a-f0-9]+:	c5 7d 7e 04 65 67 ff ff ff 	vmovd  DWORD PTR \[riz\*2-0x99\],xmm8
[ 	]*[a-f0-9]+:	c5 3f 2a 3c 65 67 ff ff ff 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[riz\*2-0x99\]
[ 	]*[a-f0-9]+:	c5 7d 7e 84 23 67 ff ff ff 	vmovd  DWORD PTR \[rbx\+riz\*1-0x99\],xmm8
[ 	]*[a-f0-9]+:	c5 3f 2a bc 23 67 ff ff ff 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[rbx\+riz\*1-0x99\]
[ 	]*[a-f0-9]+:	c5 7d 7e 84 63 67 ff ff ff 	vmovd  DWORD PTR \[rbx\+riz\*2-0x99\],xmm8
[ 	]*[a-f0-9]+:	c5 3f 2a bc 63 67 ff ff ff 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[rbx\+riz\*2-0x99\]
[ 	]*[a-f0-9]+:	c4 01 7d 7e 84 bc 67 ff ff ff 	vmovd  DWORD PTR \[r12\+r15\*4-0x99\],xmm8
[ 	]*[a-f0-9]+:	c4 01 3f 2a bc bc 67 ff ff ff 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[r12\+r15\*4-0x99\]
[ 	]*[a-f0-9]+:	c4 01 7d 7e 84 f8 67 ff ff ff 	vmovd  DWORD PTR \[r8\+r15\*8-0x99\],xmm8
[ 	]*[a-f0-9]+:	c4 01 3f 2a bc f8 67 ff ff ff 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[r8\+r15\*8-0x99\]
[ 	]*[a-f0-9]+:	c4 21 7d 7e 84 a5 67 ff ff ff 	vmovd  DWORD PTR \[rbp\+r12\*4-0x99\],xmm8
[ 	]*[a-f0-9]+:	c4 21 3f 2a bc a5 67 ff ff ff 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[rbp\+r12\*4-0x99\]
[ 	]*[a-f0-9]+:	c4 21 7d 7e 84 2c 67 ff ff ff 	vmovd  DWORD PTR \[rsp\+r13\*1-0x99\],xmm8
[ 	]*[a-f0-9]+:	c4 21 3f 2a bc 2c 67 ff ff ff 	vcvtsi2sd xmm15,xmm8,DWORD PTR \[rsp\+r13\*1-0x99\]
[ 	]*[a-f0-9]+:	c4 41 7d 7e c0       	vmovd  r8d,xmm8
[ 	]*[a-f0-9]+:	c4 41 7f 2d c0       	vcvtsd2si r8d,xmm8
[ 	]*[a-f0-9]+:	c4 41 3f 2a f8       	vcvtsi2sd xmm15,xmm8,r8d
[ 	]*[a-f0-9]+:	c4 61 ff 2d 01       	vcvtsd2si r8,QWORD PTR \[rcx\]
[ 	]*[a-f0-9]+:	c4 61 fe 2d 01       	vcvtss2si r8,DWORD PTR \[rcx\]
#pass
