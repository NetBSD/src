#as: -mevexlig=256
#objdump: -dw
#name: x86-64 EVEX non-LIG insns with -mevexlig=256

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
 +[a-f0-9]+:	62 f1 7d 08 7e 21    	vmovd  %xmm4,\(%rcx\)
 +[a-f0-9]+:	62 f1 7d 08 7e e1    	vmovd  %xmm4,%ecx
 +[a-f0-9]+:	62 f1 7d 08 6e 21    	vmovd  \(%rcx\),%xmm4
 +[a-f0-9]+:	62 f1 7d 08 6e e1    	vmovd  %ecx,%xmm4
 +[a-f0-9]+:	62 f1 fd 08 7e 21    	vmovq  %xmm4,\(%rcx\)
 +[a-f0-9]+:	62 f1 fd 08 7e e1    	vmovq  %xmm4,%rcx
 +[a-f0-9]+:	62 f1 fd 08 6e 21    	vmovq  \(%rcx\),%xmm4
 +[a-f0-9]+:	62 f1 fd 08 6e e1    	vmovq  %rcx,%xmm4
 +[a-f0-9]+:	62 f1 fe 08 7e f4    	vmovq  %xmm4,%xmm6
 +[a-f0-9]+:	62 f3 7d 08 17 c0 00 	vextractps \$0x0,%xmm0,%eax
 +[a-f0-9]+:	62 f3 7d 08 17 00 00 	vextractps \$0x0,%xmm0,\(%rax\)
 +[a-f0-9]+:	62 f3 7d 08 14 c0 00 	vpextrb \$0x0,%xmm0,%eax
 +[a-f0-9]+:	62 f3 7d 08 14 00 00 	vpextrb \$0x0,%xmm0,\(%rax\)
 +[a-f0-9]+:	62 f1 7d 08 c5 c0 00 	vpextrw \$0x0,%xmm0,%eax
 +[a-f0-9]+:	62 f3 7d 08 15 c0 00 	vpextrw \$0x0,%xmm0,%eax
 +[a-f0-9]+:	62 f3 7d 08 15 00 00 	vpextrw \$0x0,%xmm0,\(%rax\)
 +[a-f0-9]+:	62 f3 7d 08 16 c0 00 	vpextrd \$0x0,%xmm0,%eax
 +[a-f0-9]+:	62 f3 7d 08 16 00 00 	vpextrd \$0x0,%xmm0,\(%rax\)
 +[a-f0-9]+:	62 f3 fd 08 16 c0 00 	vpextrq \$0x0,%xmm0,%rax
 +[a-f0-9]+:	62 f3 fd 08 16 00 00 	vpextrq \$0x0,%xmm0,\(%rax\)
 +[a-f0-9]+:	62 f3 7d 08 21 c0 00 	vinsertps \$0x0,%xmm0,%xmm0,%xmm0
 +[a-f0-9]+:	62 f3 7d 08 21 00 00 	vinsertps \$0x0,\(%rax\),%xmm0,%xmm0
 +[a-f0-9]+:	62 f3 7d 08 20 c0 00 	vpinsrb \$0x0,%eax,%xmm0,%xmm0
 +[a-f0-9]+:	62 f3 7d 08 20 00 00 	vpinsrb \$0x0,\(%rax\),%xmm0,%xmm0
 +[a-f0-9]+:	62 f1 7d 08 c4 c0 00 	vpinsrw \$0x0,%eax,%xmm0,%xmm0
 +[a-f0-9]+:	62 f1 7d 08 c4 00 00 	vpinsrw \$0x0,\(%rax\),%xmm0,%xmm0
 +[a-f0-9]+:	62 f3 7d 08 22 c0 00 	vpinsrd \$0x0,%eax,%xmm0,%xmm0
 +[a-f0-9]+:	62 f3 7d 08 22 00 00 	vpinsrd \$0x0,\(%rax\),%xmm0,%xmm0
 +[a-f0-9]+:	62 f3 fd 08 22 c0 00 	vpinsrq \$0x0,%rax,%xmm0,%xmm0
 +[a-f0-9]+:	62 f3 fd 08 22 00 00 	vpinsrq \$0x0,\(%rax\),%xmm0,%xmm0
#pass
