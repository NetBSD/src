#source: i386.s
#objdump: -dw -Mintel
#name: i386 (Intel mode)

.*: +file format .*

Disassembly of section .text:

0+ <.text>:
[ 	]*[a-f0-9]+:	df e0                	fnstsw ax
[ 	]*[a-f0-9]+:	df e0                	fnstsw ax
[ 	]*[a-f0-9]+:	9b df e0             	fstsw  ax
[ 	]*[a-f0-9]+:	9b df e0             	fstsw  ax
[ 	]*[a-f0-9]+:	66 0f be f0          	movsx  si,al
[ 	]*[a-f0-9]+:	0f be f0             	movsx  esi,al
[ 	]*[a-f0-9]+:	0f bf f0             	movsx  esi,ax
[ 	]*[a-f0-9]+:	0f be 10             	movsx  edx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	66 0f be 10          	movsx  dx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	66 0f be 10          	movsx  dx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	0f be 10             	movsx  edx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	0f bf 10             	movsx  edx,WORD PTR \[eax\]
[ 	]*[a-f0-9]+:	0f be 10             	movsx  edx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	66 0f be 10          	movsx  dx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	0f bf 10             	movsx  edx,WORD PTR \[eax\]
[ 	]*[a-f0-9]+:	66 0f b6 f0          	movzx  si,al
[ 	]*[a-f0-9]+:	0f b6 f0             	movzx  esi,al
[ 	]*[a-f0-9]+:	0f b7 f0             	movzx  esi,ax
[ 	]*[a-f0-9]+:	0f b6 10             	movzx  edx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	66 0f b6 10          	movzx  dx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	66 0f b6 10          	movzx  dx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	0f b6 10             	movzx  edx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	0f b7 10             	movzx  edx,WORD PTR \[eax\]
[ 	]*[a-f0-9]+:	0f b6 10             	movzx  edx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	66 0f b6 10          	movzx  dx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	0f b6 10             	movzx  edx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	66 0f b6 10          	movzx  dx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	0f b7 10             	movzx  edx,WORD PTR \[eax\]
[ 	]*[a-f0-9]+:	0f c3 00             	movnti DWORD PTR \[eax\],eax
[ 	]*[a-f0-9]+:	0f c3 00             	movnti DWORD PTR \[eax\],eax
[ 	]*[a-f0-9]+:	df e0                	fnstsw ax
[ 	]*[a-f0-9]+:	df e0                	fnstsw ax
[ 	]*[a-f0-9]+:	9b df e0             	fstsw  ax
[ 	]*[a-f0-9]+:	9b df e0             	fstsw  ax
[ 	]*[a-f0-9]+:	66 0f be f0          	movsx  si,al
[ 	]*[a-f0-9]+:	0f be f0             	movsx  esi,al
[ 	]*[a-f0-9]+:	0f bf f0             	movsx  esi,ax
[ 	]*[a-f0-9]+:	0f be 10             	movsx  edx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	66 0f be 10          	movsx  dx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	0f bf 10             	movsx  edx,WORD PTR \[eax\]
[ 	]*[a-f0-9]+:	66 0f b6 f0          	movzx  si,al
[ 	]*[a-f0-9]+:	0f b6 f0             	movzx  esi,al
[ 	]*[a-f0-9]+:	0f b7 f0             	movzx  esi,ax
[ 	]*[a-f0-9]+:	0f b6 10             	movzx  edx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	66 0f b6 10          	movzx  dx,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	0f b7 10             	movzx  edx,WORD PTR \[eax\]
[ 	]*[a-f0-9]+:	f3 0f 7e 0c 24       	movq   xmm1,QWORD PTR \[esp\]
[ 	]*[a-f0-9]+:	f3 0f 7e 0c 24       	movq   xmm1,QWORD PTR \[esp\]
[ 	]*[a-f0-9]+:	66 0f d6 0c 24       	movq   QWORD PTR \[esp\],xmm1
[ 	]*[a-f0-9]+:	66 0f d6 0c 24       	movq   QWORD PTR \[esp\],xmm1
[ 	]*[a-f0-9]+:	66 0f be 00          	movsx  ax,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	0f be 00             	movsx  eax,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	0f bf 00             	movsx  eax,WORD PTR \[eax\]
[ 	]*[a-f0-9]+:	66 0f b6 00          	movzx  ax,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	0f b6 00             	movzx  eax,BYTE PTR \[eax\]
[ 	]*[a-f0-9]+:	0f b7 00             	movzx  eax,WORD PTR \[eax\]
[ 	]*[a-f0-9]+:	0f c3 00             	movnti DWORD PTR \[eax\],eax
#pass
