#source: ../x86-64-simd.s
#as: -J
#objdump: -dw -Mintel
#name: x86-64 (ILP32) SIMD (Intel mode)

.*: +file format .*

Disassembly of section .text:

0+ <_start>:
[ 	]*[a-f0-9]+:	f2 0f d0 0d 78 56 34 12 	addsubps xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 12345680 <_start\+0x12345680>
[ 	]*[a-f0-9]+:	66 0f 2f 0d 78 56 34 12 	comisd xmm1,QWORD PTR \[rip\+0x12345678\]        # 12345688 <_start\+0x12345688>
[ 	]*[a-f0-9]+:	0f 2f 0d 78 56 34 12 	comiss xmm1,DWORD PTR \[rip\+0x12345678\]        # 1234568f <_start\+0x1234568f>
[ 	]*[a-f0-9]+:	f3 0f e6 0d 78 56 34 12 	cvtdq2pd xmm1,QWORD PTR \[rip\+0x12345678\]        # 12345697 <_start\+0x12345697>
[ 	]*[a-f0-9]+:	f2 0f e6 0d 78 56 34 12 	cvtpd2dq xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 1234569f <_start\+0x1234569f>
[ 	]*[a-f0-9]+:	0f 5a 0d 78 56 34 12 	cvtps2pd xmm1,QWORD PTR \[rip\+0x12345678\]        # 123456a6 <_start\+0x123456a6>
[ 	]*[a-f0-9]+:	f3 0f 5b 0d 78 56 34 12 	cvttps2dq xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 123456ae <_start\+0x123456ae>
[ 	]*[a-f0-9]+:	f3 0f 2a c8          	cvtsi2ss xmm1,eax
[ 	]*[a-f0-9]+:	f2 0f 2a c8          	cvtsi2sd xmm1,eax
[ 	]*[a-f0-9]+:	f3 0f 2a c8          	cvtsi2ss xmm1,eax
[ 	]*[a-f0-9]+:	f2 0f 2a c8          	cvtsi2sd xmm1,eax
[ 	]*[a-f0-9]+:	f3 48 0f 2a c8       	cvtsi2ss xmm1,rax
[ 	]*[a-f0-9]+:	f2 48 0f 2a c8       	cvtsi2sd xmm1,rax
[ 	]*[a-f0-9]+:	f3 48 0f 2a c8       	cvtsi2ss xmm1,rax
[ 	]*[a-f0-9]+:	f2 48 0f 2a c8       	cvtsi2sd xmm1,rax
[ 	]*[a-f0-9]+:	f3 0f 2a 08          	cvtsi2ss xmm1,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 2a 08          	cvtsi2sd xmm1,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 2a 08          	cvtsi2ss xmm1,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 2a 08          	cvtsi2sd xmm1,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 48 0f 2a 08       	cvtsi2ss xmm1,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 48 0f 2a 08       	cvtsi2sd xmm1,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 7c 0d 78 56 34 12 	haddps xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 123456f4 <_start\+0x123456f4>
[ 	]*[a-f0-9]+:	f3 0f 7f 0d 78 56 34 12 	movdqu XMMWORD PTR \[rip\+0x12345678\],xmm1        # 123456fc <_start\+0x123456fc>
[ 	]*[a-f0-9]+:	f3 0f 6f 0d 78 56 34 12 	movdqu xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 12345704 <_start\+0x12345704>
[ 	]*[a-f0-9]+:	66 0f 17 0d 78 56 34 12 	movhpd QWORD PTR \[rip\+0x12345678\],xmm1        # 1234570c <_start\+0x1234570c>
[ 	]*[a-f0-9]+:	66 0f 16 0d 78 56 34 12 	movhpd xmm1,QWORD PTR \[rip\+0x12345678\]        # 12345714 <_start\+0x12345714>
[ 	]*[a-f0-9]+:	0f 17 0d 78 56 34 12 	movhps QWORD PTR \[rip\+0x12345678\],xmm1        # 1234571b <_start\+0x1234571b>
[ 	]*[a-f0-9]+:	0f 16 0d 78 56 34 12 	movhps xmm1,QWORD PTR \[rip\+0x12345678\]        # 12345722 <_start\+0x12345722>
[ 	]*[a-f0-9]+:	66 0f 13 0d 78 56 34 12 	movlpd QWORD PTR \[rip\+0x12345678\],xmm1        # 1234572a <_start\+0x1234572a>
[ 	]*[a-f0-9]+:	66 0f 12 0d 78 56 34 12 	movlpd xmm1,QWORD PTR \[rip\+0x12345678\]        # 12345732 <_start\+0x12345732>
[ 	]*[a-f0-9]+:	0f 13 0d 78 56 34 12 	movlps QWORD PTR \[rip\+0x12345678\],xmm1        # 12345739 <_start\+0x12345739>
[ 	]*[a-f0-9]+:	0f 12 0d 78 56 34 12 	movlps xmm1,QWORD PTR \[rip\+0x12345678\]        # 12345740 <_start\+0x12345740>
[ 	]*[a-f0-9]+:	66 0f d6 0d 78 56 34 12 	movq   QWORD PTR \[rip\+0x12345678\],xmm1        # 12345748 <_start\+0x12345748>
[ 	]*[a-f0-9]+:	f3 0f 7e 0d 78 56 34 12 	movq   xmm1,QWORD PTR \[rip\+0x12345678\]        # 12345750 <_start\+0x12345750>
[ 	]*[a-f0-9]+:	f3 0f 16 0d 78 56 34 12 	movshdup xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 12345758 <_start\+0x12345758>
[ 	]*[a-f0-9]+:	f3 0f 12 0d 78 56 34 12 	movsldup xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 12345760 <_start\+0x12345760>
[ 	]*[a-f0-9]+:	f3 0f 70 0d 78 56 34 12 90 	pshufhw xmm1,XMMWORD PTR \[rip\+0x12345678\],0x90        # 12345769 <_start\+0x12345769>
[ 	]*[a-f0-9]+:	f2 0f 70 0d 78 56 34 12 90 	pshuflw xmm1,XMMWORD PTR \[rip\+0x12345678\],0x90        # 12345772 <_start\+0x12345772>
[ 	]*[a-f0-9]+:	0f 60 0d 78 56 34 12 	punpcklbw mm1,DWORD PTR \[rip\+0x12345678\]        # 12345779 <_start\+0x12345779>
[ 	]*[a-f0-9]+:	0f 62 0d 78 56 34 12 	punpckldq mm1,DWORD PTR \[rip\+0x12345678\]        # 12345780 <_start\+0x12345780>
[ 	]*[a-f0-9]+:	0f 61 0d 78 56 34 12 	punpcklwd mm1,DWORD PTR \[rip\+0x12345678\]        # 12345787 <_start\+0x12345787>
[ 	]*[a-f0-9]+:	66 0f 60 0d 78 56 34 12 	punpcklbw xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 1234578f <_start\+0x1234578f>
[ 	]*[a-f0-9]+:	66 0f 62 0d 78 56 34 12 	punpckldq xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 12345797 <_start\+0x12345797>
[ 	]*[a-f0-9]+:	66 0f 61 0d 78 56 34 12 	punpcklwd xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 1234579f <_start\+0x1234579f>
[ 	]*[a-f0-9]+:	66 0f 6c 0d 78 56 34 12 	punpcklqdq xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 123457a7 <_start\+0x123457a7>
[ 	]*[a-f0-9]+:	66 0f 2e 0d 78 56 34 12 	ucomisd xmm1,QWORD PTR \[rip\+0x12345678\]        # 123457af <_start\+0x123457af>
[ 	]*[a-f0-9]+:	0f 2e 0d 78 56 34 12 	ucomiss xmm1,DWORD PTR \[rip\+0x12345678\]        # 123457b6 <_start\+0x123457b6>
[ 	]*[a-f0-9]+:	f2 0f c2 00 00       	cmpeqsd xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f c2 00 00       	cmpeqss xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 2a 00          	cvtpi2pd xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	0f 2a 00             	cvtpi2ps xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	0f 2d 00             	cvtps2pi mm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 2d 00          	cvtsd2si eax,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 48 0f 2d 00       	cvtsd2si rax,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 2c 00          	cvttsd2si eax,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 48 0f 2c 00       	cvttsd2si rax,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 5a 00          	cvtsd2ss xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 5a 00          	cvtss2sd xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 2d 00          	cvtss2si eax,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 48 0f 2d 00       	cvtss2si rax,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 2c 00          	cvttss2si eax,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 48 0f 2c 00       	cvttss2si rax,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 5e 00          	divsd  xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 5e 00          	divss  xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 5f 00          	maxsd  xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 5f 00          	maxss  xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 5d 00          	minss  xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 5d 00          	minss  xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 2b 00          	movntsd QWORD PTR \[rax\],xmm0
[ 	]*[a-f0-9]+:	f3 0f 2b 00          	movntss DWORD PTR \[rax\],xmm0
[ 	]*[a-f0-9]+:	f2 0f 10 00          	movsd  xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 11 00          	movsd  QWORD PTR \[rax\],xmm0
[ 	]*[a-f0-9]+:	f3 0f 10 00          	movss  xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 11 00          	movss  DWORD PTR \[rax\],xmm0
[ 	]*[a-f0-9]+:	f2 0f 59 00          	mulsd  xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 59 00          	mulss  xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 53 00          	rcpss  xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 3a 0b 00 00    	roundsd xmm0,QWORD PTR \[rax\],0x0
[ 	]*[a-f0-9]+:	66 0f 3a 0a 00 00    	roundss xmm0,DWORD PTR \[rax\],0x0
[ 	]*[a-f0-9]+:	f3 0f 52 00          	rsqrtss xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 51 00          	sqrtsd xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 51 00          	sqrtss xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 5c 00          	subsd  xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 5c 00          	subss  xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 20 00       	pmovsxbw xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 21 00       	pmovsxbd xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 22 00       	pmovsxbq xmm0,WORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 23 00       	pmovsxwd xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 24 00       	pmovsxwq xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 25 00       	pmovsxdq xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 30 00       	pmovzxbw xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 31 00       	pmovzxbd xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 32 00       	pmovzxbq xmm0,WORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 33 00       	pmovzxwd xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 34 00       	pmovzxwq xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 35 00       	pmovzxdq xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 3a 21 00 00    	insertps xmm0,DWORD PTR \[rax\],0x0
[ 	]*[a-f0-9]+:	66 0f 15 08          	unpckhpd xmm1,XMMWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	0f 15 08             	unpckhps xmm1,XMMWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 14 08          	unpcklpd xmm1,XMMWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	0f 14 08             	unpcklps xmm1,XMMWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f c2 f7 10       	cmpss  xmm6,xmm7,0x10
[ 	]*[a-f0-9]+:	f3 0f c2 38 10       	cmpss  xmm7,DWORD PTR \[rax\],0x10
[ 	]*[a-f0-9]+:	f2 0f c2 f7 10       	cmpsd  xmm6,xmm7,0x10
[ 	]*[a-f0-9]+:	f2 0f c2 38 10       	cmpsd  xmm7,QWORD PTR \[rax\],0x10
[ 	]*[a-f0-9]+:	0f d4 c1             	paddq  mm0,mm1
[ 	]*[a-f0-9]+:	0f d4 00             	paddq  mm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f d4 c1          	paddq  xmm0,xmm1
[ 	]*[a-f0-9]+:	66 0f d4 00          	paddq  xmm0,XMMWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	0f fb c1             	psubq  mm0,mm1
[ 	]*[a-f0-9]+:	0f fb 00             	psubq  mm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f fb c1          	psubq  xmm0,xmm1
[ 	]*[a-f0-9]+:	66 0f fb 00          	psubq  xmm0,XMMWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	0f f4 c1             	pmuludq mm0,mm1
[ 	]*[a-f0-9]+:	0f f4 00             	pmuludq mm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f f4 c1          	pmuludq xmm0,xmm1
[ 	]*[a-f0-9]+:	66 0f f4 00          	pmuludq xmm0,XMMWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f d0 0d 78 56 34 12 	addsubps xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 123458e8 <_start\+0x123458e8>
[ 	]*[a-f0-9]+:	66 0f 2f 0d 78 56 34 12 	comisd xmm1,QWORD PTR \[rip\+0x12345678\]        # 123458f0 <_start\+0x123458f0>
[ 	]*[a-f0-9]+:	0f 2f 0d 78 56 34 12 	comiss xmm1,DWORD PTR \[rip\+0x12345678\]        # 123458f7 <_start\+0x123458f7>
[ 	]*[a-f0-9]+:	f3 0f e6 0d 78 56 34 12 	cvtdq2pd xmm1,QWORD PTR \[rip\+0x12345678\]        # 123458ff <_start\+0x123458ff>
[ 	]*[a-f0-9]+:	f2 0f e6 0d 78 56 34 12 	cvtpd2dq xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 12345907 <_start\+0x12345907>
[ 	]*[a-f0-9]+:	0f 5a 0d 78 56 34 12 	cvtps2pd xmm1,QWORD PTR \[rip\+0x12345678\]        # 1234590e <_start\+0x1234590e>
[ 	]*[a-f0-9]+:	f3 0f 5b 0d 78 56 34 12 	cvttps2dq xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 12345916 <_start\+0x12345916>
[ 	]*[a-f0-9]+:	f3 0f 2a c8          	cvtsi2ss xmm1,eax
[ 	]*[a-f0-9]+:	f2 0f 2a c8          	cvtsi2sd xmm1,eax
[ 	]*[a-f0-9]+:	f3 0f 2a c8          	cvtsi2ss xmm1,eax
[ 	]*[a-f0-9]+:	f2 0f 2a c8          	cvtsi2sd xmm1,eax
[ 	]*[a-f0-9]+:	f3 48 0f 2a c8       	cvtsi2ss xmm1,rax
[ 	]*[a-f0-9]+:	f2 48 0f 2a c8       	cvtsi2sd xmm1,rax
[ 	]*[a-f0-9]+:	f3 48 0f 2a c8       	cvtsi2ss xmm1,rax
[ 	]*[a-f0-9]+:	f2 48 0f 2a c8       	cvtsi2sd xmm1,rax
[ 	]*[a-f0-9]+:	f3 0f 2a 08          	cvtsi2ss xmm1,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 2a 08          	cvtsi2sd xmm1,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 2a 08          	cvtsi2ss xmm1,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 2a 08          	cvtsi2sd xmm1,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 48 0f 2a 08       	cvtsi2ss xmm1,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 48 0f 2a 08       	cvtsi2sd xmm1,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 48 0f 2a 08       	cvtsi2ss xmm1,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 48 0f 2a 08       	cvtsi2sd xmm1,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 7c 0d 78 56 34 12 	haddps xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 12345966 <_start\+0x12345966>
[ 	]*[a-f0-9]+:	f3 0f 7f 0d 78 56 34 12 	movdqu XMMWORD PTR \[rip\+0x12345678\],xmm1        # 1234596e <_start\+0x1234596e>
[ 	]*[a-f0-9]+:	f3 0f 6f 0d 78 56 34 12 	movdqu xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 12345976 <_start\+0x12345976>
[ 	]*[a-f0-9]+:	66 0f 17 0d 78 56 34 12 	movhpd QWORD PTR \[rip\+0x12345678\],xmm1        # 1234597e <_start\+0x1234597e>
[ 	]*[a-f0-9]+:	66 0f 16 0d 78 56 34 12 	movhpd xmm1,QWORD PTR \[rip\+0x12345678\]        # 12345986 <_start\+0x12345986>
[ 	]*[a-f0-9]+:	0f 17 0d 78 56 34 12 	movhps QWORD PTR \[rip\+0x12345678\],xmm1        # 1234598d <_start\+0x1234598d>
[ 	]*[a-f0-9]+:	0f 16 0d 78 56 34 12 	movhps xmm1,QWORD PTR \[rip\+0x12345678\]        # 12345994 <_start\+0x12345994>
[ 	]*[a-f0-9]+:	66 0f 13 0d 78 56 34 12 	movlpd QWORD PTR \[rip\+0x12345678\],xmm1        # 1234599c <_start\+0x1234599c>
[ 	]*[a-f0-9]+:	66 0f 12 0d 78 56 34 12 	movlpd xmm1,QWORD PTR \[rip\+0x12345678\]        # 123459a4 <_start\+0x123459a4>
[ 	]*[a-f0-9]+:	0f 13 0d 78 56 34 12 	movlps QWORD PTR \[rip\+0x12345678\],xmm1        # 123459ab <_start\+0x123459ab>
[ 	]*[a-f0-9]+:	0f 12 0d 78 56 34 12 	movlps xmm1,QWORD PTR \[rip\+0x12345678\]        # 123459b2 <_start\+0x123459b2>
[ 	]*[a-f0-9]+:	66 0f d6 0d 78 56 34 12 	movq   QWORD PTR \[rip\+0x12345678\],xmm1        # 123459ba <_start\+0x123459ba>
[ 	]*[a-f0-9]+:	f3 0f 7e 0d 78 56 34 12 	movq   xmm1,QWORD PTR \[rip\+0x12345678\]        # 123459c2 <_start\+0x123459c2>
[ 	]*[a-f0-9]+:	f3 0f 16 0d 78 56 34 12 	movshdup xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 123459ca <_start\+0x123459ca>
[ 	]*[a-f0-9]+:	f3 0f 12 0d 78 56 34 12 	movsldup xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 123459d2 <_start\+0x123459d2>
[ 	]*[a-f0-9]+:	f3 0f 70 0d 78 56 34 12 90 	pshufhw xmm1,XMMWORD PTR \[rip\+0x12345678\],0x90        # 123459db <_start\+0x123459db>
[ 	]*[a-f0-9]+:	f2 0f 70 0d 78 56 34 12 90 	pshuflw xmm1,XMMWORD PTR \[rip\+0x12345678\],0x90        # 123459e4 <_start\+0x123459e4>
[ 	]*[a-f0-9]+:	0f 60 0d 78 56 34 12 	punpcklbw mm1,DWORD PTR \[rip\+0x12345678\]        # 123459eb <_start\+0x123459eb>
[ 	]*[a-f0-9]+:	0f 62 0d 78 56 34 12 	punpckldq mm1,DWORD PTR \[rip\+0x12345678\]        # 123459f2 <_start\+0x123459f2>
[ 	]*[a-f0-9]+:	0f 61 0d 78 56 34 12 	punpcklwd mm1,DWORD PTR \[rip\+0x12345678\]        # 123459f9 <_start\+0x123459f9>
[ 	]*[a-f0-9]+:	66 0f 60 0d 78 56 34 12 	punpcklbw xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 12345a01 <_start\+0x12345a01>
[ 	]*[a-f0-9]+:	66 0f 62 0d 78 56 34 12 	punpckldq xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 12345a09 <_start\+0x12345a09>
[ 	]*[a-f0-9]+:	66 0f 61 0d 78 56 34 12 	punpcklwd xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 12345a11 <_start\+0x12345a11>
[ 	]*[a-f0-9]+:	66 0f 6c 0d 78 56 34 12 	punpcklqdq xmm1,XMMWORD PTR \[rip\+0x12345678\]        # 12345a19 <_start\+0x12345a19>
[ 	]*[a-f0-9]+:	66 0f 2e 0d 78 56 34 12 	ucomisd xmm1,QWORD PTR \[rip\+0x12345678\]        # 12345a21 <_start\+0x12345a21>
[ 	]*[a-f0-9]+:	0f 2e 0d 78 56 34 12 	ucomiss xmm1,DWORD PTR \[rip\+0x12345678\]        # 12345a28 <_start\+0x12345a28>
[ 	]*[a-f0-9]+:	f2 0f c2 00 00       	cmpeqsd xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f c2 00 00       	cmpeqss xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 2a 00          	cvtpi2pd xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	0f 2a 00             	cvtpi2ps xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	0f 2d 00             	cvtps2pi mm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 2d 00          	cvtsd2si eax,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 48 0f 2d 00       	cvtsd2si rax,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 2c 00          	cvttsd2si eax,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 48 0f 2c 00       	cvttsd2si rax,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 5a 00          	cvtsd2ss xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 5a 00          	cvtss2sd xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 2d 00          	cvtss2si eax,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 48 0f 2d 00       	cvtss2si rax,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 2c 00          	cvttss2si eax,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 48 0f 2c 00       	cvttss2si rax,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 5e 00          	divsd  xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 5e 00          	divss  xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 5f 00          	maxsd  xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 5f 00          	maxss  xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 5d 00          	minss  xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 5d 00          	minss  xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 2b 00          	movntsd QWORD PTR \[rax\],xmm0
[ 	]*[a-f0-9]+:	f3 0f 2b 00          	movntss DWORD PTR \[rax\],xmm0
[ 	]*[a-f0-9]+:	f2 0f 10 00          	movsd  xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 11 00          	movsd  QWORD PTR \[rax\],xmm0
[ 	]*[a-f0-9]+:	f3 0f 10 00          	movss  xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 11 00          	movss  DWORD PTR \[rax\],xmm0
[ 	]*[a-f0-9]+:	f2 0f 59 00          	mulsd  xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 59 00          	mulss  xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 53 00          	rcpss  xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 3a 0b 00 00    	roundsd xmm0,QWORD PTR \[rax\],0x0
[ 	]*[a-f0-9]+:	66 0f 3a 0a 00 00    	roundss xmm0,DWORD PTR \[rax\],0x0
[ 	]*[a-f0-9]+:	f3 0f 52 00          	rsqrtss xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 51 00          	sqrtsd xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 51 00          	sqrtss xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f2 0f 5c 00          	subsd  xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f 5c 00          	subss  xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 20 00       	pmovsxbw xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 21 00       	pmovsxbd xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 22 00       	pmovsxbq xmm0,WORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 23 00       	pmovsxwd xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 24 00       	pmovsxwq xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 25 00       	pmovsxdq xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 30 00       	pmovzxbw xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 31 00       	pmovzxbd xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 32 00       	pmovzxbq xmm0,WORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 33 00       	pmovzxwd xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 34 00       	pmovzxwq xmm0,DWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 38 35 00       	pmovzxdq xmm0,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 3a 21 00 00    	insertps xmm0,DWORD PTR \[rax\],0x0
[ 	]*[a-f0-9]+:	66 0f 15 00          	unpckhpd xmm0,XMMWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	0f 15 00             	unpckhps xmm0,XMMWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f 14 00          	unpcklpd xmm0,XMMWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	0f 14 00             	unpcklps xmm0,XMMWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	f3 0f c2 f7 10       	cmpss  xmm6,xmm7,0x10
[ 	]*[a-f0-9]+:	f3 0f c2 38 10       	cmpss  xmm7,DWORD PTR \[rax\],0x10
[ 	]*[a-f0-9]+:	f2 0f c2 f7 10       	cmpsd  xmm6,xmm7,0x10
[ 	]*[a-f0-9]+:	f2 0f c2 38 10       	cmpsd  xmm7,QWORD PTR \[rax\],0x10
[ 	]*[a-f0-9]+:	0f d4 08             	paddq  mm1,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	0f d4 08             	paddq  mm1,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f d4 08          	paddq  xmm1,XMMWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f d4 08          	paddq  xmm1,XMMWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	0f fb 08             	psubq  mm1,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	0f fb 08             	psubq  mm1,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f fb 08          	psubq  xmm1,XMMWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f fb 08          	psubq  xmm1,XMMWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	0f f4 08             	pmuludq mm1,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	0f f4 08             	pmuludq mm1,QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f f4 08          	pmuludq xmm1,XMMWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 0f f4 08          	pmuludq xmm1,XMMWORD PTR \[rax\]
#pass
