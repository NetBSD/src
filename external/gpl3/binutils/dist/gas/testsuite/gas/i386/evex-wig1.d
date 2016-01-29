#as: -mevexwig=1
#objdump: -dw
#name: i386 AVX512 wig insns
#source: evex-wig.s

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 f5    	vpmovsxbd %xmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 21 f5    	vpmovsxbd %xmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 31    	vpmovsxbd \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 b4 f4 c0 1d fe ff 	vpmovsxbd -0x1e240\(%esp,%esi,8\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 72 7f 	vpmovsxbd 0x7f0\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 b2 00 08 00 00 	vpmovsxbd 0x800\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 72 80 	vpmovsxbd -0x800\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 b2 f0 f7 ff ff 	vpmovsxbd -0x810\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 f5    	vpmovsxbq %xmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 22 f5    	vpmovsxbq %xmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 31    	vpmovsxbq \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 b4 f4 c0 1d fe ff 	vpmovsxbq -0x1e240\(%esp,%esi,8\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 72 7f 	vpmovsxbq 0x3f8\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 b2 00 04 00 00 	vpmovsxbq 0x400\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 72 80 	vpmovsxbq -0x400\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 b2 f8 fb ff ff 	vpmovsxbq -0x408\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 f5    	vpmovsxwd %ymm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 23 f5    	vpmovsxwd %ymm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 31    	vpmovsxwd \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 b4 f4 c0 1d fe ff 	vpmovsxwd -0x1e240\(%esp,%esi,8\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 72 7f 	vpmovsxwd 0xfe0\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 b2 00 10 00 00 	vpmovsxwd 0x1000\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 72 80 	vpmovsxwd -0x1000\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 b2 e0 ef ff ff 	vpmovsxwd -0x1020\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 f5    	vpmovsxwq %xmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 24 f5    	vpmovsxwq %xmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 31    	vpmovsxwq \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 b4 f4 c0 1d fe ff 	vpmovsxwq -0x1e240\(%esp,%esi,8\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 72 7f 	vpmovsxwq 0x7f0\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 b2 00 08 00 00 	vpmovsxwq 0x800\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 72 80 	vpmovsxwq -0x800\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 b2 f0 f7 ff ff 	vpmovsxwq -0x810\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 f5    	vpmovzxbd %xmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 31 f5    	vpmovzxbd %xmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 31    	vpmovzxbd \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 b4 f4 c0 1d fe ff 	vpmovzxbd -0x1e240\(%esp,%esi,8\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 72 7f 	vpmovzxbd 0x7f0\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 b2 00 08 00 00 	vpmovzxbd 0x800\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 72 80 	vpmovzxbd -0x800\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 b2 f0 f7 ff ff 	vpmovzxbd -0x810\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 f5    	vpmovzxbq %xmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 32 f5    	vpmovzxbq %xmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 31    	vpmovzxbq \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 b4 f4 c0 1d fe ff 	vpmovzxbq -0x1e240\(%esp,%esi,8\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 72 7f 	vpmovzxbq 0x3f8\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 b2 00 04 00 00 	vpmovzxbq 0x400\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 72 80 	vpmovzxbq -0x400\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 b2 f8 fb ff ff 	vpmovzxbq -0x408\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 f5    	vpmovzxwd %ymm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 33 f5    	vpmovzxwd %ymm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 31    	vpmovzxwd \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 b4 f4 c0 1d fe ff 	vpmovzxwd -0x1e240\(%esp,%esi,8\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 72 7f 	vpmovzxwd 0xfe0\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 b2 00 10 00 00 	vpmovzxwd 0x1000\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 72 80 	vpmovzxwd -0x1000\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 b2 e0 ef ff ff 	vpmovzxwd -0x1020\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 f5    	vpmovzxwq %xmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 34 f5    	vpmovzxwq %xmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 31    	vpmovzxwq \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 b4 f4 c0 1d fe ff 	vpmovzxwq -0x1e240\(%esp,%esi,8\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 72 7f 	vpmovzxwq 0x7f0\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 b2 00 08 00 00 	vpmovzxwq 0x800\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 72 80 	vpmovzxwq -0x800\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 b2 f0 f7 ff ff 	vpmovzxwq -0x810\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 f5    	vpmovsxbd %xmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 21 f5    	vpmovsxbd %xmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 31    	vpmovsxbd \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 b4 f4 c0 1d fe ff 	vpmovsxbd -0x1e240\(%esp,%esi,8\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 72 7f 	vpmovsxbd 0x7f0\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 b2 00 08 00 00 	vpmovsxbd 0x800\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 72 80 	vpmovsxbd -0x800\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 21 b2 f0 f7 ff ff 	vpmovsxbd -0x810\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 f5    	vpmovsxbq %xmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 22 f5    	vpmovsxbq %xmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 31    	vpmovsxbq \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 b4 f4 c0 1d fe ff 	vpmovsxbq -0x1e240\(%esp,%esi,8\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 72 7f 	vpmovsxbq 0x3f8\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 b2 00 04 00 00 	vpmovsxbq 0x400\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 72 80 	vpmovsxbq -0x400\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 22 b2 f8 fb ff ff 	vpmovsxbq -0x408\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 f5    	vpmovsxwd %ymm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 23 f5    	vpmovsxwd %ymm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 31    	vpmovsxwd \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 b4 f4 c0 1d fe ff 	vpmovsxwd -0x1e240\(%esp,%esi,8\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 72 7f 	vpmovsxwd 0xfe0\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 b2 00 10 00 00 	vpmovsxwd 0x1000\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 72 80 	vpmovsxwd -0x1000\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 23 b2 e0 ef ff ff 	vpmovsxwd -0x1020\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 f5    	vpmovsxwq %xmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 24 f5    	vpmovsxwq %xmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 31    	vpmovsxwq \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 b4 f4 c0 1d fe ff 	vpmovsxwq -0x1e240\(%esp,%esi,8\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 72 7f 	vpmovsxwq 0x7f0\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 b2 00 08 00 00 	vpmovsxwq 0x800\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 72 80 	vpmovsxwq -0x800\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 24 b2 f0 f7 ff ff 	vpmovsxwq -0x810\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 f5    	vpmovzxbd %xmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 31 f5    	vpmovzxbd %xmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 31    	vpmovzxbd \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 b4 f4 c0 1d fe ff 	vpmovzxbd -0x1e240\(%esp,%esi,8\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 72 7f 	vpmovzxbd 0x7f0\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 b2 00 08 00 00 	vpmovzxbd 0x800\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 72 80 	vpmovzxbd -0x800\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 31 b2 f0 f7 ff ff 	vpmovzxbd -0x810\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 f5    	vpmovzxbq %xmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 32 f5    	vpmovzxbq %xmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 31    	vpmovzxbq \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 b4 f4 c0 1d fe ff 	vpmovzxbq -0x1e240\(%esp,%esi,8\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 72 7f 	vpmovzxbq 0x3f8\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 b2 00 04 00 00 	vpmovzxbq 0x400\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 72 80 	vpmovzxbq -0x400\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 32 b2 f8 fb ff ff 	vpmovzxbq -0x408\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 f5    	vpmovzxwd %ymm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 33 f5    	vpmovzxwd %ymm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 31    	vpmovzxwd \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 b4 f4 c0 1d fe ff 	vpmovzxwd -0x1e240\(%esp,%esi,8\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 72 7f 	vpmovzxwd 0xfe0\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 b2 00 10 00 00 	vpmovzxwd 0x1000\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 72 80 	vpmovzxwd -0x1000\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 33 b2 e0 ef ff ff 	vpmovzxwd -0x1020\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 f5    	vpmovzxwq %xmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 34 f5    	vpmovzxwq %xmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 31    	vpmovzxwq \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 b4 f4 c0 1d fe ff 	vpmovzxwq -0x1e240\(%esp,%esi,8\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 72 7f 	vpmovzxwq 0x7f0\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 b2 00 08 00 00 	vpmovzxwq 0x800\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 72 80 	vpmovzxwq -0x800\(%edx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd 4f 34 b2 f0 f7 ff ff 	vpmovzxwq -0x810\(%edx\),%zmm6\{%k7\}
#pass
