#as:
#objdump: -dw -Msuffix
#name: x86_64 AVX512BW opts insns
#source: x86-64-avx512bw-opts.s

.*: +file format .*


Disassembly of section \.text:

0+ <_start>:
[ 	]*[a-f0-9]+:[ 	]*62 91 fd 08 c5 c5 ab[ 	]*vpextrw \$0xab,%xmm29,%rax
[ 	]*[a-f0-9]+:[ 	]*62 63 fd 08 15 e8 ab[ 	]*vpextrw\.s \$0xab,%xmm29,%rax
[ 	]*[a-f0-9]+:[ 	]*62 91 fd 08 c5 c5 7b[ 	]*vpextrw \$0x7b,%xmm29,%rax
[ 	]*[a-f0-9]+:[ 	]*62 63 fd 08 15 e8 7b[ 	]*vpextrw\.s \$0x7b,%xmm29,%rax
[ 	]*[a-f0-9]+:[ 	]*62 11 fd 08 c5 c5 7b[ 	]*vpextrw \$0x7b,%xmm29,%r8
[ 	]*[a-f0-9]+:[ 	]*62 43 fd 08 15 e8 7b[ 	]*vpextrw\.s \$0x7b,%xmm29,%r8
[ 	]*[a-f0-9]+:[ 	]*62 91 fd 08 c5 c5 ab[ 	]*vpextrw \$0xab,%xmm29,%rax
[ 	]*[a-f0-9]+:[ 	]*62 63 fd 08 15 e8 ab[ 	]*vpextrw\.s \$0xab,%xmm29,%rax
[ 	]*[a-f0-9]+:[ 	]*62 91 fd 08 c5 c5 7b[ 	]*vpextrw \$0x7b,%xmm29,%rax
[ 	]*[a-f0-9]+:[ 	]*62 63 fd 08 15 e8 7b[ 	]*vpextrw\.s \$0x7b,%xmm29,%rax
[ 	]*[a-f0-9]+:[ 	]*62 11 fd 08 c5 c5 7b[ 	]*vpextrw \$0x7b,%xmm29,%r8
[ 	]*[a-f0-9]+:[ 	]*62 43 fd 08 15 e8 7b[ 	]*vpextrw\.s \$0x7b,%xmm29,%r8
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 48 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 48 7f ee[ 	]*vmovdqu8\.s %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 4f 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 4f 7f ee[ 	]*vmovdqu8\.s %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f cf 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f cf 7f ee[ 	]*vmovdqu8\.s %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 48 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 48 7f ee[ 	]*vmovdqu8\.s %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 4f 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 4f 7f ee[ 	]*vmovdqu8\.s %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f cf 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f cf 7f ee[ 	]*vmovdqu8\.s %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 48 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 48 7f ee[ 	]*vmovdqu16\.s %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 4f 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 4f 7f ee[ 	]*vmovdqu16\.s %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff cf 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff cf 7f ee[ 	]*vmovdqu16\.s %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 48 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 48 7f ee[ 	]*vmovdqu16\.s %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 4f 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 4f 7f ee[ 	]*vmovdqu16\.s %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff cf 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff cf 7f ee[ 	]*vmovdqu16\.s %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 91 fd 08 c5 c5 ab[ 	]*vpextrw \$0xab,%xmm29,%rax
[ 	]*[a-f0-9]+:[ 	]*62 63 fd 08 15 e8 ab[ 	]*vpextrw\.s \$0xab,%xmm29,%rax
[ 	]*[a-f0-9]+:[ 	]*62 91 fd 08 c5 c5 7b[ 	]*vpextrw \$0x7b,%xmm29,%rax
[ 	]*[a-f0-9]+:[ 	]*62 63 fd 08 15 e8 7b[ 	]*vpextrw\.s \$0x7b,%xmm29,%rax
[ 	]*[a-f0-9]+:[ 	]*62 11 fd 08 c5 c5 7b[ 	]*vpextrw \$0x7b,%xmm29,%r8
[ 	]*[a-f0-9]+:[ 	]*62 43 fd 08 15 e8 7b[ 	]*vpextrw\.s \$0x7b,%xmm29,%r8
[ 	]*[a-f0-9]+:[ 	]*62 91 fd 08 c5 c5 ab[ 	]*vpextrw \$0xab,%xmm29,%rax
[ 	]*[a-f0-9]+:[ 	]*62 63 fd 08 15 e8 ab[ 	]*vpextrw\.s \$0xab,%xmm29,%rax
[ 	]*[a-f0-9]+:[ 	]*62 91 fd 08 c5 c5 7b[ 	]*vpextrw \$0x7b,%xmm29,%rax
[ 	]*[a-f0-9]+:[ 	]*62 63 fd 08 15 e8 7b[ 	]*vpextrw\.s \$0x7b,%xmm29,%rax
[ 	]*[a-f0-9]+:[ 	]*62 11 fd 08 c5 c5 7b[ 	]*vpextrw \$0x7b,%xmm29,%r8
[ 	]*[a-f0-9]+:[ 	]*62 43 fd 08 15 e8 7b[ 	]*vpextrw\.s \$0x7b,%xmm29,%r8
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 48 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 48 7f ee[ 	]*vmovdqu8\.s %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 4f 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 4f 7f ee[ 	]*vmovdqu8\.s %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f cf 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f cf 7f ee[ 	]*vmovdqu8\.s %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 48 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 48 7f ee[ 	]*vmovdqu8\.s %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 4f 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 4f 7f ee[ 	]*vmovdqu8\.s %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f cf 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f cf 7f ee[ 	]*vmovdqu8\.s %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 48 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 48 7f ee[ 	]*vmovdqu16\.s %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 4f 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 4f 7f ee[ 	]*vmovdqu16\.s %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff cf 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff cf 7f ee[ 	]*vmovdqu16\.s %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 48 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 48 7f ee[ 	]*vmovdqu16\.s %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 4f 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 4f 7f ee[ 	]*vmovdqu16\.s %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff cf 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff cf 7f ee[ 	]*vmovdqu16\.s %zmm29,%zmm30\{%k7\}\{z\}
#pass
