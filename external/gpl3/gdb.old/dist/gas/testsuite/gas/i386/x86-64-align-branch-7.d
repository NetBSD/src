#as: -mbranches-within-32B-boundaries -malign-branch-prefix-size=4
#objdump: -dw

.*: +file format .*

Disassembly of section .text:

0+ <foo>:
 +[a-f0-9]+:	2e 66 0f 3a 60 00 03 	pcmpestrm \$0x3,%cs:\(%rax\),%xmm0
 +[a-f0-9]+:	2e 2e 48 89 e5       	cs cs mov %rsp,%rbp
 +[a-f0-9]+:	89 7d f8             	mov    %edi,-0x8\(%rbp\)
 +[a-f0-9]+:	89 75 f4             	mov    %esi,-0xc\(%rbp\)
 +[a-f0-9]+:	89 75 f4             	mov    %esi,-0xc\(%rbp\)
 +[a-f0-9]+:	89 75 f4             	mov    %esi,-0xc\(%rbp\)
 +[a-f0-9]+:	64 89 04 25 01 00 00 00 	mov    %eax,%fs:0x1
 +[a-f0-9]+:	a8 04                	test   \$0x4,%al
 +[a-f0-9]+:	70 dc                	jo     0 <foo>
#pass
