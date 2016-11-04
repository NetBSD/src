#objdump: -dwMsuffix
#name: x86-64 rep prefix (with suffixes)

.*: +file format .*

Disassembly of section .text:

0+ <foo>:
[ 	]*[a-f0-9]+:	0f 01 c8             	monitor %rax,%rcx,%rdx
[ 	]*[a-f0-9]+:	0f 01 c9             	mwait  %rax,%rcx
[ 	]*[a-f0-9]+:	0f 01 c1             	vmcall 
[ 	]*[a-f0-9]+:	0f 01 c2             	vmlaunch 
[ 	]*[a-f0-9]+:	0f 01 c3             	vmresume 
[ 	]*[a-f0-9]+:	0f 01 c4             	vmxoff 
[ 	]*[a-f0-9]+:	66 cf                	iretw  
[ 	]*[a-f0-9]+:	cf                   	iretl  
[ 	]*[a-f0-9]+:	cf                   	iretl  
[ 	]*[a-f0-9]+:	48 cf                	iretq  
[ 	]*[a-f0-9]+:	0f 07                	sysretl 
[ 	]*[a-f0-9]+:	0f 07                	sysretl 
[ 	]*[a-f0-9]+:	48 0f 07             	sysretq 
[ 	]*[a-f0-9]+:	66 cf                	iretw  
[ 	]*[a-f0-9]+:	cf                   	iretl  
[ 	]*[a-f0-9]+:	cf                   	iretl  
[ 	]*[a-f0-9]+:	48 cf                	iretq  
[ 	]*[a-f0-9]+:	0f 07                	sysretl 
[ 	]*[a-f0-9]+:	0f 07                	sysretl 
[ 	]*[a-f0-9]+:	48 0f 07             	sysretq 
#pass
