#as: -mevexrcig=ru
#objdump: -dw -Mintel
#name: x86_64 AVX512DQ rcig insns (Intel disassembly)
#source: x86-64-avx512dq-rcig.s

.*: +file format .*


Disassembly of section \.text:

0+ <_start>:
[ 	]*[a-f0-9]+:[ 	]*62 03 95 50 50 f4 ab[ 	]*vrangepd zmm30,zmm29,zmm28,\{sae\},0xab
[ 	]*[a-f0-9]+:[ 	]*62 03 95 50 50 f4 7b[ 	]*vrangepd zmm30,zmm29,zmm28,\{sae\},0x7b
[ 	]*[a-f0-9]+:[ 	]*62 03 15 50 50 f4 ab[ 	]*vrangeps zmm30,zmm29,zmm28,\{sae\},0xab
[ 	]*[a-f0-9]+:[ 	]*62 03 15 50 50 f4 7b[ 	]*vrangeps zmm30,zmm29,zmm28,\{sae\},0x7b
[ 	]*[a-f0-9]+:[ 	]*62 03 95 50 51 f4 ab[ 	]*vrangesd xmm30,xmm29,xmm28,\{sae\},0xab
[ 	]*[a-f0-9]+:[ 	]*62 03 95 50 51 f4 7b[ 	]*vrangesd xmm30,xmm29,xmm28,\{sae\},0x7b
[ 	]*[a-f0-9]+:[ 	]*62 03 15 50 51 f4 ab[ 	]*vrangess xmm30,xmm29,xmm28,\{sae\},0xab
[ 	]*[a-f0-9]+:[ 	]*62 03 15 50 51 f4 7b[ 	]*vrangess xmm30,xmm29,xmm28,\{sae\},0x7b
[ 	]*[a-f0-9]+:[ 	]*62 03 fd 58 56 f5 ab[ 	]*vreducepd zmm30,zmm29,\{sae\},0xab
[ 	]*[a-f0-9]+:[ 	]*62 03 fd 58 56 f5 7b[ 	]*vreducepd zmm30,zmm29,\{sae\},0x7b
[ 	]*[a-f0-9]+:[ 	]*62 03 7d 58 56 f5 ab[ 	]*vreduceps zmm30,zmm29,\{sae\},0xab
[ 	]*[a-f0-9]+:[ 	]*62 03 7d 58 56 f5 7b[ 	]*vreduceps zmm30,zmm29,\{sae\},0x7b
[ 	]*[a-f0-9]+:[ 	]*62 03 95 50 57 f4 ab[ 	]*vreducesd xmm30,xmm29,xmm28,\{sae\},0xab
[ 	]*[a-f0-9]+:[ 	]*62 03 95 50 57 f4 7b[ 	]*vreducesd xmm30,xmm29,xmm28,\{sae\},0x7b
[ 	]*[a-f0-9]+:[ 	]*62 03 15 50 57 f4 ab[ 	]*vreducess xmm30,xmm29,xmm28,\{sae\},0xab
[ 	]*[a-f0-9]+:[ 	]*62 03 15 50 57 f4 7b[ 	]*vreducess xmm30,xmm29,xmm28,\{sae\},0x7b
[ 	]*[a-f0-9]+:[ 	]*62 01 fd 58 7a f5[ 	]*vcvttpd2qq zmm30,zmm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 01 fd 58 78 f5[ 	]*vcvttpd2uqq zmm30,zmm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7d 58 7a f5[ 	]*vcvttps2qq zmm30,ymm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7d 58 78 f5[ 	]*vcvttps2uqq zmm30,ymm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 03 95 50 50 f4 ab[ 	]*vrangepd zmm30,zmm29,zmm28,\{sae\},0xab
[ 	]*[a-f0-9]+:[ 	]*62 03 95 50 50 f4 7b[ 	]*vrangepd zmm30,zmm29,zmm28,\{sae\},0x7b
[ 	]*[a-f0-9]+:[ 	]*62 03 15 50 50 f4 ab[ 	]*vrangeps zmm30,zmm29,zmm28,\{sae\},0xab
[ 	]*[a-f0-9]+:[ 	]*62 03 15 50 50 f4 7b[ 	]*vrangeps zmm30,zmm29,zmm28,\{sae\},0x7b
[ 	]*[a-f0-9]+:[ 	]*62 03 95 50 51 f4 ab[ 	]*vrangesd xmm30,xmm29,xmm28,\{sae\},0xab
[ 	]*[a-f0-9]+:[ 	]*62 03 95 50 51 f4 7b[ 	]*vrangesd xmm30,xmm29,xmm28,\{sae\},0x7b
[ 	]*[a-f0-9]+:[ 	]*62 03 15 50 51 f4 ab[ 	]*vrangess xmm30,xmm29,xmm28,\{sae\},0xab
[ 	]*[a-f0-9]+:[ 	]*62 03 15 50 51 f4 7b[ 	]*vrangess xmm30,xmm29,xmm28,\{sae\},0x7b
[ 	]*[a-f0-9]+:[ 	]*62 03 fd 58 56 f5 ab[ 	]*vreducepd zmm30,zmm29,\{sae\},0xab
[ 	]*[a-f0-9]+:[ 	]*62 03 fd 58 56 f5 7b[ 	]*vreducepd zmm30,zmm29,\{sae\},0x7b
[ 	]*[a-f0-9]+:[ 	]*62 03 7d 58 56 f5 ab[ 	]*vreduceps zmm30,zmm29,\{sae\},0xab
[ 	]*[a-f0-9]+:[ 	]*62 03 7d 58 56 f5 7b[ 	]*vreduceps zmm30,zmm29,\{sae\},0x7b
[ 	]*[a-f0-9]+:[ 	]*62 03 95 50 57 f4 ab[ 	]*vreducesd xmm30,xmm29,xmm28,\{sae\},0xab
[ 	]*[a-f0-9]+:[ 	]*62 03 95 50 57 f4 7b[ 	]*vreducesd xmm30,xmm29,xmm28,\{sae\},0x7b
[ 	]*[a-f0-9]+:[ 	]*62 03 15 50 57 f4 ab[ 	]*vreducess xmm30,xmm29,xmm28,\{sae\},0xab
[ 	]*[a-f0-9]+:[ 	]*62 03 15 50 57 f4 7b[ 	]*vreducess xmm30,xmm29,xmm28,\{sae\},0x7b
[ 	]*[a-f0-9]+:[ 	]*62 01 fd 58 7a f5[ 	]*vcvttpd2qq zmm30,zmm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 01 fd 58 78 f5[ 	]*vcvttpd2uqq zmm30,zmm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7d 58 7a f5[ 	]*vcvttps2qq zmm30,ymm29,\{sae\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7d 58 78 f5[ 	]*vcvttps2uqq zmm30,ymm29,\{sae\}
#pass
