#as:
#objdump: -dwMintel
#name: i386 AVX512PF insns (Intel disassembly)
#source: avx512pf.s

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 8c fd 7b 00 00 00 	vgatherpf0dpd ZMMWORD PTR \[ebp\+ymm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 8c fd 7b 00 00 00 	vgatherpf0dpd ZMMWORD PTR \[ebp\+ymm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 4c 38 20 	vgatherpf0dpd ZMMWORD PTR \[eax\+ymm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 8c b9 00 04 00 00 	vgatherpf0dpd ZMMWORD PTR \[ecx\+ymm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 8c fd 7b 00 00 00 	vgatherpf0dps ZMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 8c fd 7b 00 00 00 	vgatherpf0dps ZMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 4c 38 40 	vgatherpf0dps ZMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 8c b9 00 04 00 00 	vgatherpf0dps ZMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 8c fd 7b 00 00 00 	vgatherpf0qpd ZMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 8c fd 7b 00 00 00 	vgatherpf0qpd ZMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 4c 38 20 	vgatherpf0qpd ZMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 8c b9 00 04 00 00 	vgatherpf0qpd ZMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 8c fd 7b 00 00 00 	vgatherpf0qps YMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 8c fd 7b 00 00 00 	vgatherpf0qps YMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 4c 38 40 	vgatherpf0qps YMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 8c b9 00 04 00 00 	vgatherpf0qps YMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 94 fd 7b 00 00 00 	vgatherpf1dpd ZMMWORD PTR \[ebp\+ymm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 94 fd 7b 00 00 00 	vgatherpf1dpd ZMMWORD PTR \[ebp\+ymm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 54 38 20 	vgatherpf1dpd ZMMWORD PTR \[eax\+ymm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 94 b9 00 04 00 00 	vgatherpf1dpd ZMMWORD PTR \[ecx\+ymm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 94 fd 7b 00 00 00 	vgatherpf1dps ZMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 94 fd 7b 00 00 00 	vgatherpf1dps ZMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 54 38 40 	vgatherpf1dps ZMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 94 b9 00 04 00 00 	vgatherpf1dps ZMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 94 fd 7b 00 00 00 	vgatherpf1qpd ZMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 94 fd 7b 00 00 00 	vgatherpf1qpd ZMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 54 38 20 	vgatherpf1qpd ZMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 94 b9 00 04 00 00 	vgatherpf1qpd ZMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 94 fd 7b 00 00 00 	vgatherpf1qps YMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 94 fd 7b 00 00 00 	vgatherpf1qps YMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 54 38 40 	vgatherpf1qps YMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 94 b9 00 04 00 00 	vgatherpf1qps YMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 ac fd 7b 00 00 00 	vscatterpf0dpd ZMMWORD PTR \[ebp\+ymm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 ac fd 7b 00 00 00 	vscatterpf0dpd ZMMWORD PTR \[ebp\+ymm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 6c 38 20 	vscatterpf0dpd ZMMWORD PTR \[eax\+ymm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 ac b9 00 04 00 00 	vscatterpf0dpd ZMMWORD PTR \[ecx\+ymm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 ac fd 7b 00 00 00 	vscatterpf0dps ZMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 ac fd 7b 00 00 00 	vscatterpf0dps ZMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 6c 38 40 	vscatterpf0dps ZMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 ac b9 00 04 00 00 	vscatterpf0dps ZMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 ac fd 7b 00 00 00 	vscatterpf0qpd ZMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 ac fd 7b 00 00 00 	vscatterpf0qpd ZMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 6c 38 20 	vscatterpf0qpd ZMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 ac b9 00 04 00 00 	vscatterpf0qpd ZMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 ac fd 7b 00 00 00 	vscatterpf0qps YMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 ac fd 7b 00 00 00 	vscatterpf0qps YMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 6c 38 40 	vscatterpf0qps YMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 ac b9 00 04 00 00 	vscatterpf0qps YMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 b4 fd 7b 00 00 00 	vscatterpf1dpd ZMMWORD PTR \[ebp\+ymm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 b4 fd 7b 00 00 00 	vscatterpf1dpd ZMMWORD PTR \[ebp\+ymm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 74 38 20 	vscatterpf1dpd ZMMWORD PTR \[eax\+ymm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 b4 b9 00 04 00 00 	vscatterpf1dpd ZMMWORD PTR \[ecx\+ymm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 b4 fd 7b 00 00 00 	vscatterpf1dps ZMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 b4 fd 7b 00 00 00 	vscatterpf1dps ZMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 74 38 40 	vscatterpf1dps ZMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 b4 b9 00 04 00 00 	vscatterpf1dps ZMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 b4 fd 7b 00 00 00 	vscatterpf1qpd ZMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 b4 fd 7b 00 00 00 	vscatterpf1qpd ZMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 74 38 20 	vscatterpf1qpd ZMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 b4 b9 00 04 00 00 	vscatterpf1qpd ZMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 b4 fd 7b 00 00 00 	vscatterpf1qps YMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 b4 fd 7b 00 00 00 	vscatterpf1qps YMMWORD PTR \[ebp\+zmm7\*8\+0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 74 38 40 	vscatterpf1qps YMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 b4 b9 00 04 00 00 	vscatterpf1qps YMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 8c fd 85 ff ff ff 	vgatherpf0dpd ZMMWORD PTR \[ebp\+ymm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 8c fd 85 ff ff ff 	vgatherpf0dpd ZMMWORD PTR \[ebp\+ymm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 4c 38 20 	vgatherpf0dpd ZMMWORD PTR \[eax\+ymm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 8c b9 00 04 00 00 	vgatherpf0dpd ZMMWORD PTR \[ecx\+ymm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 8c fd 85 ff ff ff 	vgatherpf0dps ZMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 8c fd 85 ff ff ff 	vgatherpf0dps ZMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 4c 38 40 	vgatherpf0dps ZMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 8c b9 00 04 00 00 	vgatherpf0dps ZMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 8c fd 85 ff ff ff 	vgatherpf0qpd ZMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 8c fd 85 ff ff ff 	vgatherpf0qpd ZMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 4c 38 20 	vgatherpf0qpd ZMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 8c b9 00 04 00 00 	vgatherpf0qpd ZMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 8c fd 85 ff ff ff 	vgatherpf0qps YMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 8c fd 85 ff ff ff 	vgatherpf0qps YMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 4c 38 40 	vgatherpf0qps YMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 8c b9 00 04 00 00 	vgatherpf0qps YMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 94 fd 85 ff ff ff 	vgatherpf1dpd ZMMWORD PTR \[ebp\+ymm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 94 fd 85 ff ff ff 	vgatherpf1dpd ZMMWORD PTR \[ebp\+ymm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 54 38 20 	vgatherpf1dpd ZMMWORD PTR \[eax\+ymm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 94 b9 00 04 00 00 	vgatherpf1dpd ZMMWORD PTR \[ecx\+ymm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 94 fd 85 ff ff ff 	vgatherpf1dps ZMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 94 fd 85 ff ff ff 	vgatherpf1dps ZMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 54 38 40 	vgatherpf1dps ZMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 94 b9 00 04 00 00 	vgatherpf1dps ZMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 94 fd 85 ff ff ff 	vgatherpf1qpd ZMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 94 fd 85 ff ff ff 	vgatherpf1qpd ZMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 54 38 20 	vgatherpf1qpd ZMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 94 b9 00 04 00 00 	vgatherpf1qpd ZMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 94 fd 85 ff ff ff 	vgatherpf1qps YMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 94 fd 85 ff ff ff 	vgatherpf1qps YMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 54 38 40 	vgatherpf1qps YMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 94 b9 00 04 00 00 	vgatherpf1qps YMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 ac fd 85 ff ff ff 	vscatterpf0dpd ZMMWORD PTR \[ebp\+ymm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 ac fd 85 ff ff ff 	vscatterpf0dpd ZMMWORD PTR \[ebp\+ymm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 6c 38 20 	vscatterpf0dpd ZMMWORD PTR \[eax\+ymm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 ac b9 00 04 00 00 	vscatterpf0dpd ZMMWORD PTR \[ecx\+ymm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 ac fd 85 ff ff ff 	vscatterpf0dps ZMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 ac fd 85 ff ff ff 	vscatterpf0dps ZMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 6c 38 40 	vscatterpf0dps ZMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 ac b9 00 04 00 00 	vscatterpf0dps ZMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 ac fd 85 ff ff ff 	vscatterpf0qpd ZMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 ac fd 85 ff ff ff 	vscatterpf0qpd ZMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 6c 38 20 	vscatterpf0qpd ZMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 ac b9 00 04 00 00 	vscatterpf0qpd ZMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 ac fd 85 ff ff ff 	vscatterpf0qps YMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 ac fd 85 ff ff ff 	vscatterpf0qps YMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 6c 38 40 	vscatterpf0qps YMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 ac b9 00 04 00 00 	vscatterpf0qps YMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 b4 fd 85 ff ff ff 	vscatterpf1dpd ZMMWORD PTR \[ebp\+ymm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 b4 fd 85 ff ff ff 	vscatterpf1dpd ZMMWORD PTR \[ebp\+ymm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 74 38 20 	vscatterpf1dpd ZMMWORD PTR \[eax\+ymm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c6 b4 b9 00 04 00 00 	vscatterpf1dpd ZMMWORD PTR \[ecx\+ymm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 b4 fd 85 ff ff ff 	vscatterpf1dps ZMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 b4 fd 85 ff ff ff 	vscatterpf1dps ZMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 74 38 40 	vscatterpf1dps ZMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c6 b4 b9 00 04 00 00 	vscatterpf1dps ZMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 b4 fd 85 ff ff ff 	vscatterpf1qpd ZMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 b4 fd 85 ff ff ff 	vscatterpf1qpd ZMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 74 38 20 	vscatterpf1qpd ZMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 fd 49 c7 b4 b9 00 04 00 00 	vscatterpf1qpd ZMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 b4 fd 85 ff ff ff 	vscatterpf1qps YMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 b4 fd 85 ff ff ff 	vscatterpf1qps YMMWORD PTR \[ebp\+zmm7\*8-0x7b\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 74 38 40 	vscatterpf1qps YMMWORD PTR \[eax\+zmm7\*1\+0x100\]\{k1\}
[ 	]*[a-f0-9]+:	62 f2 7d 49 c7 b4 b9 00 04 00 00 	vscatterpf1qps YMMWORD PTR \[ecx\+zmm7\*4\+0x400\]\{k1\}
#pass
