#as: -mevexwig=1
#objdump: -dw
#name: x86_64 AVX512VL/VPCLMULQDQ wig insns
#source: x86-64-avx512vl_vpclmulqdq-wig.s

.*: +file format .*


Disassembly of section \.text:

0+ <_start>:
[ 	]*[a-f0-9]+:[ 	]*62 a3 d5 00 44 cf ab[ 	]*vpclmulqdq \$0xab,%xmm23,%xmm21,%xmm17
[ 	]*[a-f0-9]+:[ 	]*62 a3 d5 00 44 8c f0 23 01 00 00 7b[ 	]*vpclmulqdq \$0x7b,0x123\(%rax,%r14,8\),%xmm21,%xmm17
[ 	]*[a-f0-9]+:[ 	]*62 e3 d5 00 44 4a 7f 7b[ 	]*vpclmulqdq \$0x7b,0x7f0\(%rdx\),%xmm21,%xmm17
[ 	]*[a-f0-9]+:[ 	]*62 a3 ed 20 44 fb ab[ 	]*vpclmulqdq \$0xab,%ymm19,%ymm18,%ymm23
[ 	]*[a-f0-9]+:[ 	]*62 a3 ed 20 44 bc f0 23 01 00 00 7b[ 	]*vpclmulqdq \$0x7b,0x123\(%rax,%r14,8\),%ymm18,%ymm23
[ 	]*[a-f0-9]+:[ 	]*62 e3 ed 20 44 7a 7f 7b[ 	]*vpclmulqdq \$0x7b,0xfe0\(%rdx\),%ymm18,%ymm23
[ 	]*[a-f0-9]+:[ 	]*62 a3 d5 00 44 cf ab[ 	]*vpclmulqdq \$0xab,%xmm23,%xmm21,%xmm17
[ 	]*[a-f0-9]+:[ 	]*62 a3 d5 00 44 8c f0 23 01 00 00 7b[ 	]*vpclmulqdq \$0x7b,0x123\(%rax,%r14,8\),%xmm21,%xmm17
[ 	]*[a-f0-9]+:[ 	]*62 e3 d5 00 44 4a 7f 7b[ 	]*vpclmulqdq \$0x7b,0x7f0\(%rdx\),%xmm21,%xmm17
[ 	]*[a-f0-9]+:[ 	]*62 a3 ed 20 44 fb ab[ 	]*vpclmulqdq \$0xab,%ymm19,%ymm18,%ymm23
[ 	]*[a-f0-9]+:[ 	]*62 a3 ed 20 44 bc f0 23 01 00 00 7b[ 	]*vpclmulqdq \$0x7b,0x123\(%rax,%r14,8\),%ymm18,%ymm23
[ 	]*[a-f0-9]+:[ 	]*62 e3 ed 20 44 7a 7f 7b[ 	]*vpclmulqdq \$0x7b,0xfe0\(%rdx\),%ymm18,%ymm23
[ 	]*[a-f0-9]+:[ 	]*62 a3 cd 00 44 d1 ab[ 	]*vpclmulqdq \$0xab,%xmm17,%xmm22,%xmm18
[ 	]*[a-f0-9]+:[ 	]*62 a3 cd 00 44 94 f0 34 12 00 00 7b[ 	]*vpclmulqdq \$0x7b,0x1234\(%rax,%r14,8\),%xmm22,%xmm18
[ 	]*[a-f0-9]+:[ 	]*62 e3 cd 00 44 52 7f 7b[ 	]*vpclmulqdq \$0x7b,0x7f0\(%rdx\),%xmm22,%xmm18
[ 	]*[a-f0-9]+:[ 	]*62 23 b5 20 44 d7 ab[ 	]*vpclmulqdq \$0xab,%ymm23,%ymm25,%ymm26
[ 	]*[a-f0-9]+:[ 	]*62 23 b5 20 44 94 f0 34 12 00 00 7b[ 	]*vpclmulqdq \$0x7b,0x1234\(%rax,%r14,8\),%ymm25,%ymm26
[ 	]*[a-f0-9]+:[ 	]*62 63 b5 20 44 52 7f 7b[ 	]*vpclmulqdq \$0x7b,0xfe0\(%rdx\),%ymm25,%ymm26
[ 	]*[a-f0-9]+:[ 	]*62 a3 cd 00 44 d1 ab[ 	]*vpclmulqdq \$0xab,%xmm17,%xmm22,%xmm18
[ 	]*[a-f0-9]+:[ 	]*62 a3 cd 00 44 94 f0 34 12 00 00 7b[ 	]*vpclmulqdq \$0x7b,0x1234\(%rax,%r14,8\),%xmm22,%xmm18
[ 	]*[a-f0-9]+:[ 	]*62 e3 cd 00 44 52 7f 7b[ 	]*vpclmulqdq \$0x7b,0x7f0\(%rdx\),%xmm22,%xmm18
[ 	]*[a-f0-9]+:[ 	]*62 23 b5 20 44 d7 ab[ 	]*vpclmulqdq \$0xab,%ymm23,%ymm25,%ymm26
[ 	]*[a-f0-9]+:[ 	]*62 23 b5 20 44 94 f0 34 12 00 00 7b[ 	]*vpclmulqdq \$0x7b,0x1234\(%rax,%r14,8\),%ymm25,%ymm26
[ 	]*[a-f0-9]+:[ 	]*62 63 b5 20 44 52 7f 7b[ 	]*vpclmulqdq \$0x7b,0xfe0\(%rdx\),%ymm25,%ymm26
#pass
