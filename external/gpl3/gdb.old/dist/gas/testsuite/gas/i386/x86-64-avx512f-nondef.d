#as: 
#objdump: -dw
#name: x86-64 AVX512F insns with nondefault values in ignored bits

.*: +file format .*


Disassembly of section .text:

0+ <.text>:
[ 	]*[a-f0-9]+:	62 f3 d5 1f 0b f4 7b 	vrndscalesd \$0x7b,\{sae\},%xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f3 d5 5f 0b f4 7b 	vrndscalesd \$0x7b,\{sae\},%xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 55 1f 3b f4    	vpminud %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 c2 55 1f 3b f4    	vpminud %zmm12,%zmm5,%zmm22\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 7e 48 31 72 7f 	vpmovdb %zmm6,0x7f0\(%rdx\)
[ 	]*[a-f0-9]+:	62                   	vpmovdb %zmm6,\(bad\)
[ 	]*[a-f0-9]+:	f2 7e 58             	bnd jle (0x7d|7d <.text\+0x7d>)
[ 	]*[a-f0-9]+:	31 72 7f             	xor    %esi,0x7f\(%rdx\)
#pass
