#as: -mevexrcig=rz
#objdump: -dw -Mintel
#name: x86_64 AVX512ER rcig insns (Intel disassembly)
#source: x86-64-avx512er-rcig.s

.*: +file format .*


Disassembly of section \.text:

0+ <_start>:
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 78 c8 f5[ 	]*vexp2ps zmm30,zmm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 fd 78 c8 f5[ 	]*vexp2pd zmm30,zmm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 78 ca f5[ 	]*vrcp28ps zmm30,zmm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 fd 78 ca f5[ 	]*vrcp28pd zmm30,zmm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 70 cb f4[ 	]*vrcp28ss xmm30,xmm29,xmm28,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 70 cb f4[ 	]*vrcp28sd xmm30,xmm29,xmm28,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 78 cc f5[ 	]*vrsqrt28ps zmm30,zmm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 fd 78 cc f5[ 	]*vrsqrt28pd zmm30,zmm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 70 cd f4[ 	]*vrsqrt28ss xmm30,xmm29,xmm28,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 70 cd f4[ 	]*vrsqrt28sd xmm30,xmm29,xmm28,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 78 c8 f5[ 	]*vexp2ps zmm30,zmm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 fd 78 c8 f5[ 	]*vexp2pd zmm30,zmm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 78 ca f5[ 	]*vrcp28ps zmm30,zmm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 fd 78 ca f5[ 	]*vrcp28pd zmm30,zmm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 70 cb f4[ 	]*vrcp28ss xmm30,xmm29,xmm28,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 70 cb f4[ 	]*vrcp28sd xmm30,xmm29,xmm28,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 78 cc f5[ 	]*vrsqrt28ps zmm30,zmm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 fd 78 cc f5[ 	]*vrsqrt28pd zmm30,zmm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 70 cd f4[ 	]*vrsqrt28ss xmm30,xmm29,xmm28,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 70 cd f4[ 	]*vrsqrt28sd xmm30,xmm29,xmm28,\{sae\}
#pass
