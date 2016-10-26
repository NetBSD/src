#source: x86-64-mwaitx.s
#as: -march=bdver4
#objdump: -dw
#name: x86_64 monitorx and mwaitx insn

.*: +file format .*


Disassembly of section \.text:

0000000000000000 <_start>:
[ 	]*[a-f0-9]+:	0f 01 fa             	monitorx %rax,%rcx,%rdx
[ 	]*[a-f0-9]+:	67 0f 01 fa          	monitorx %eax,%rcx,%rdx
[ 	]*[a-f0-9]+:	0f 01 fa             	monitorx %rax,%rcx,%rdx
[ 	]*[a-f0-9]+:	0f 01 fb             	mwaitx %rax,%rcx,%rbx
[ 	]*[a-f0-9]+:	0f 01 fb             	mwaitx %rax,%rcx,%rbx
#pass
