#as: -mevexwig=1
#objdump: -dwMintel
#name: i386 AVX512 wig insns (Intel disassembly)
#source: evex-wig.s

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 f5    	vpmovsxbd zmm6\{k7\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd cf 21 f5    	vpmovsxbd zmm6\{k7\}\{z\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 31    	vpmovsxbd zmm6\{k7\},XMMWORD PTR \[ecx\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 b4 f4 c0 1d fe ff 	vpmovsxbd zmm6\{k7\},XMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 72 7f 	vpmovsxbd zmm6\{k7\},XMMWORD PTR \[edx\+0x7f0\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 b2 00 08 00 00 	vpmovsxbd zmm6\{k7\},XMMWORD PTR \[edx\+0x800\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 72 80 	vpmovsxbd zmm6\{k7\},XMMWORD PTR \[edx-0x800\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 b2 f0 f7 ff ff 	vpmovsxbd zmm6\{k7\},XMMWORD PTR \[edx-0x810\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 f5    	vpmovsxbq zmm6\{k7\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd cf 22 f5    	vpmovsxbq zmm6\{k7\}\{z\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 31    	vpmovsxbq zmm6\{k7\},QWORD PTR \[ecx\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 b4 f4 c0 1d fe ff 	vpmovsxbq zmm6\{k7\},QWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 72 7f 	vpmovsxbq zmm6\{k7\},QWORD PTR \[edx\+0x3f8\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 b2 00 04 00 00 	vpmovsxbq zmm6\{k7\},QWORD PTR \[edx\+0x400\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 72 80 	vpmovsxbq zmm6\{k7\},QWORD PTR \[edx-0x400\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 b2 f8 fb ff ff 	vpmovsxbq zmm6\{k7\},QWORD PTR \[edx-0x408\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 f5    	vpmovsxwd zmm6\{k7\},ymm5
[ 	]*[a-f0-9]+:	62 f2 fd cf 23 f5    	vpmovsxwd zmm6\{k7\}\{z\},ymm5
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 31    	vpmovsxwd zmm6\{k7\},YMMWORD PTR \[ecx\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 b4 f4 c0 1d fe ff 	vpmovsxwd zmm6\{k7\},YMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 72 7f 	vpmovsxwd zmm6\{k7\},YMMWORD PTR \[edx\+0xfe0\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 b2 00 10 00 00 	vpmovsxwd zmm6\{k7\},YMMWORD PTR \[edx\+0x1000\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 72 80 	vpmovsxwd zmm6\{k7\},YMMWORD PTR \[edx-0x1000\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 b2 e0 ef ff ff 	vpmovsxwd zmm6\{k7\},YMMWORD PTR \[edx-0x1020\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 f5    	vpmovsxwq zmm6\{k7\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd cf 24 f5    	vpmovsxwq zmm6\{k7\}\{z\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 31    	vpmovsxwq zmm6\{k7\},XMMWORD PTR \[ecx\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 b4 f4 c0 1d fe ff 	vpmovsxwq zmm6\{k7\},XMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 72 7f 	vpmovsxwq zmm6\{k7\},XMMWORD PTR \[edx\+0x7f0\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 b2 00 08 00 00 	vpmovsxwq zmm6\{k7\},XMMWORD PTR \[edx\+0x800\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 72 80 	vpmovsxwq zmm6\{k7\},XMMWORD PTR \[edx-0x800\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 b2 f0 f7 ff ff 	vpmovsxwq zmm6\{k7\},XMMWORD PTR \[edx-0x810\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 f5    	vpmovzxbd zmm6\{k7\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd cf 31 f5    	vpmovzxbd zmm6\{k7\}\{z\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 31    	vpmovzxbd zmm6\{k7\},XMMWORD PTR \[ecx\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 b4 f4 c0 1d fe ff 	vpmovzxbd zmm6\{k7\},XMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 72 7f 	vpmovzxbd zmm6\{k7\},XMMWORD PTR \[edx\+0x7f0\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 b2 00 08 00 00 	vpmovzxbd zmm6\{k7\},XMMWORD PTR \[edx\+0x800\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 72 80 	vpmovzxbd zmm6\{k7\},XMMWORD PTR \[edx-0x800\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 b2 f0 f7 ff ff 	vpmovzxbd zmm6\{k7\},XMMWORD PTR \[edx-0x810\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 f5    	vpmovzxbq zmm6\{k7\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd cf 32 f5    	vpmovzxbq zmm6\{k7\}\{z\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 31    	vpmovzxbq zmm6\{k7\},QWORD PTR \[ecx\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 b4 f4 c0 1d fe ff 	vpmovzxbq zmm6\{k7\},QWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 72 7f 	vpmovzxbq zmm6\{k7\},QWORD PTR \[edx\+0x3f8\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 b2 00 04 00 00 	vpmovzxbq zmm6\{k7\},QWORD PTR \[edx\+0x400\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 72 80 	vpmovzxbq zmm6\{k7\},QWORD PTR \[edx-0x400\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 b2 f8 fb ff ff 	vpmovzxbq zmm6\{k7\},QWORD PTR \[edx-0x408\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 f5    	vpmovzxwd zmm6\{k7\},ymm5
[ 	]*[a-f0-9]+:	62 f2 fd cf 33 f5    	vpmovzxwd zmm6\{k7\}\{z\},ymm5
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 31    	vpmovzxwd zmm6\{k7\},YMMWORD PTR \[ecx\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 b4 f4 c0 1d fe ff 	vpmovzxwd zmm6\{k7\},YMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 72 7f 	vpmovzxwd zmm6\{k7\},YMMWORD PTR \[edx\+0xfe0\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 b2 00 10 00 00 	vpmovzxwd zmm6\{k7\},YMMWORD PTR \[edx\+0x1000\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 72 80 	vpmovzxwd zmm6\{k7\},YMMWORD PTR \[edx-0x1000\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 b2 e0 ef ff ff 	vpmovzxwd zmm6\{k7\},YMMWORD PTR \[edx-0x1020\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 f5    	vpmovzxwq zmm6\{k7\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd cf 34 f5    	vpmovzxwq zmm6\{k7\}\{z\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 31    	vpmovzxwq zmm6\{k7\},XMMWORD PTR \[ecx\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 b4 f4 c0 1d fe ff 	vpmovzxwq zmm6\{k7\},XMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 72 7f 	vpmovzxwq zmm6\{k7\},XMMWORD PTR \[edx\+0x7f0\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 b2 00 08 00 00 	vpmovzxwq zmm6\{k7\},XMMWORD PTR \[edx\+0x800\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 72 80 	vpmovzxwq zmm6\{k7\},XMMWORD PTR \[edx-0x800\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 b2 f0 f7 ff ff 	vpmovzxwq zmm6\{k7\},XMMWORD PTR \[edx-0x810\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 f5    	vpmovsxbd zmm6\{k7\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd cf 21 f5    	vpmovsxbd zmm6\{k7\}\{z\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 31    	vpmovsxbd zmm6\{k7\},XMMWORD PTR \[ecx\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 b4 f4 c0 1d fe ff 	vpmovsxbd zmm6\{k7\},XMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 72 7f 	vpmovsxbd zmm6\{k7\},XMMWORD PTR \[edx\+0x7f0\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 b2 00 08 00 00 	vpmovsxbd zmm6\{k7\},XMMWORD PTR \[edx\+0x800\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 72 80 	vpmovsxbd zmm6\{k7\},XMMWORD PTR \[edx-0x800\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 b2 f0 f7 ff ff 	vpmovsxbd zmm6\{k7\},XMMWORD PTR \[edx-0x810\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 f5    	vpmovsxbq zmm6\{k7\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd cf 22 f5    	vpmovsxbq zmm6\{k7\}\{z\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 31    	vpmovsxbq zmm6\{k7\},QWORD PTR \[ecx\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 b4 f4 c0 1d fe ff 	vpmovsxbq zmm6\{k7\},QWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 72 7f 	vpmovsxbq zmm6\{k7\},QWORD PTR \[edx\+0x3f8\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 b2 00 04 00 00 	vpmovsxbq zmm6\{k7\},QWORD PTR \[edx\+0x400\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 72 80 	vpmovsxbq zmm6\{k7\},QWORD PTR \[edx-0x400\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 b2 f8 fb ff ff 	vpmovsxbq zmm6\{k7\},QWORD PTR \[edx-0x408\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 f5    	vpmovsxwd zmm6\{k7\},ymm5
[ 	]*[a-f0-9]+:	62 f2 fd cf 23 f5    	vpmovsxwd zmm6\{k7\}\{z\},ymm5
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 31    	vpmovsxwd zmm6\{k7\},YMMWORD PTR \[ecx\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 b4 f4 c0 1d fe ff 	vpmovsxwd zmm6\{k7\},YMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 72 7f 	vpmovsxwd zmm6\{k7\},YMMWORD PTR \[edx\+0xfe0\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 b2 00 10 00 00 	vpmovsxwd zmm6\{k7\},YMMWORD PTR \[edx\+0x1000\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 72 80 	vpmovsxwd zmm6\{k7\},YMMWORD PTR \[edx-0x1000\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 b2 e0 ef ff ff 	vpmovsxwd zmm6\{k7\},YMMWORD PTR \[edx-0x1020\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 f5    	vpmovsxwq zmm6\{k7\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd cf 24 f5    	vpmovsxwq zmm6\{k7\}\{z\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 31    	vpmovsxwq zmm6\{k7\},XMMWORD PTR \[ecx\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 b4 f4 c0 1d fe ff 	vpmovsxwq zmm6\{k7\},XMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 72 7f 	vpmovsxwq zmm6\{k7\},XMMWORD PTR \[edx\+0x7f0\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 b2 00 08 00 00 	vpmovsxwq zmm6\{k7\},XMMWORD PTR \[edx\+0x800\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 72 80 	vpmovsxwq zmm6\{k7\},XMMWORD PTR \[edx-0x800\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 b2 f0 f7 ff ff 	vpmovsxwq zmm6\{k7\},XMMWORD PTR \[edx-0x810\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 f5    	vpmovzxbd zmm6\{k7\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd cf 31 f5    	vpmovzxbd zmm6\{k7\}\{z\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 31    	vpmovzxbd zmm6\{k7\},XMMWORD PTR \[ecx\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 b4 f4 c0 1d fe ff 	vpmovzxbd zmm6\{k7\},XMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 72 7f 	vpmovzxbd zmm6\{k7\},XMMWORD PTR \[edx\+0x7f0\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 b2 00 08 00 00 	vpmovzxbd zmm6\{k7\},XMMWORD PTR \[edx\+0x800\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 72 80 	vpmovzxbd zmm6\{k7\},XMMWORD PTR \[edx-0x800\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 b2 f0 f7 ff ff 	vpmovzxbd zmm6\{k7\},XMMWORD PTR \[edx-0x810\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 f5    	vpmovzxbq zmm6\{k7\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd cf 32 f5    	vpmovzxbq zmm6\{k7\}\{z\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 31    	vpmovzxbq zmm6\{k7\},QWORD PTR \[ecx\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 b4 f4 c0 1d fe ff 	vpmovzxbq zmm6\{k7\},QWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 72 7f 	vpmovzxbq zmm6\{k7\},QWORD PTR \[edx\+0x3f8\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 b2 00 04 00 00 	vpmovzxbq zmm6\{k7\},QWORD PTR \[edx\+0x400\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 72 80 	vpmovzxbq zmm6\{k7\},QWORD PTR \[edx-0x400\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 b2 f8 fb ff ff 	vpmovzxbq zmm6\{k7\},QWORD PTR \[edx-0x408\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 f5    	vpmovzxwd zmm6\{k7\},ymm5
[ 	]*[a-f0-9]+:	62 f2 fd cf 33 f5    	vpmovzxwd zmm6\{k7\}\{z\},ymm5
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 31    	vpmovzxwd zmm6\{k7\},YMMWORD PTR \[ecx\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 b4 f4 c0 1d fe ff 	vpmovzxwd zmm6\{k7\},YMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 72 7f 	vpmovzxwd zmm6\{k7\},YMMWORD PTR \[edx\+0xfe0\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 b2 00 10 00 00 	vpmovzxwd zmm6\{k7\},YMMWORD PTR \[edx\+0x1000\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 72 80 	vpmovzxwd zmm6\{k7\},YMMWORD PTR \[edx-0x1000\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 b2 e0 ef ff ff 	vpmovzxwd zmm6\{k7\},YMMWORD PTR \[edx-0x1020\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 f5    	vpmovzxwq zmm6\{k7\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd cf 34 f5    	vpmovzxwq zmm6\{k7\}\{z\},xmm5
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 31    	vpmovzxwq zmm6\{k7\},XMMWORD PTR \[ecx\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 b4 f4 c0 1d fe ff 	vpmovzxwq zmm6\{k7\},XMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 72 7f 	vpmovzxwq zmm6\{k7\},XMMWORD PTR \[edx\+0x7f0\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 b2 00 08 00 00 	vpmovzxwq zmm6\{k7\},XMMWORD PTR \[edx\+0x800\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 72 80 	vpmovzxwq zmm6\{k7\},XMMWORD PTR \[edx-0x800\]
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 b2 f0 f7 ff ff 	vpmovzxwq zmm6\{k7\},XMMWORD PTR \[edx-0x810\]
#pass
