#objdump: -dw
#name: x86-64 x86-64-sse5

.*: +file format .*

Disassembly of section .text:

0000000000000000 <foo>:
[ 	]+0:[ 	]+0f 24 02 d3 10[ 	]+fmaddss %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+5:[ 	]+0f 24 02 52 10 04[ 	]+fmaddss 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+b:[ 	]+0f 24 02 52 18 04[ 	]+fmaddss %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+11:[ 	]+0f 24 06 d3 10[ 	]+fmaddss %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+16:[ 	]+0f 24 06 52 10 04[ 	]+fmaddss %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1c:[ 	]+0f 24 06 52 18 04[ 	]+fmaddss %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+22:[ 	]+0f 24 03 d3 10[ 	]+fmaddsd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+27:[ 	]+0f 24 03 52 10 04[ 	]+fmaddsd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+2d:[ 	]+0f 24 03 52 18 04[ 	]+fmaddsd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+33:[ 	]+0f 24 07 d3 10[ 	]+fmaddsd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+38:[ 	]+0f 24 07 52 10 04[ 	]+fmaddsd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3e:[ 	]+0f 24 07 52 18 04[ 	]+fmaddsd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+44:[ 	]+0f 24 00 d3 10[ 	]+fmaddps %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+49:[ 	]+0f 24 00 52 10 04[ 	]+fmaddps 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+4f:[ 	]+0f 24 00 52 18 04[ 	]+fmaddps %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+55:[ 	]+0f 24 04 d3 10[ 	]+fmaddps %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+5a:[ 	]+0f 24 04 52 10 04[ 	]+fmaddps %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+60:[ 	]+0f 24 04 52 18 04[ 	]+fmaddps %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+66:[ 	]+0f 24 01 d3 10[ 	]+fmaddpd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+6b:[ 	]+0f 24 01 52 10 04[ 	]+fmaddpd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+71:[ 	]+0f 24 01 52 18 04[ 	]+fmaddpd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+77:[ 	]+0f 24 05 d3 10[ 	]+fmaddpd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+7c:[ 	]+0f 24 05 52 10 04[ 	]+fmaddpd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+82:[ 	]+0f 24 05 52 18 04[ 	]+fmaddpd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+88:[ 	]+0f 24 0a d3 10[ 	]+fmsubss %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+8d:[ 	]+0f 24 0a 52 10 04[ 	]+fmsubss 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+93:[ 	]+0f 24 0a 52 18 04[ 	]+fmsubss %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+99:[ 	]+0f 24 0e d3 10[ 	]+fmsubss %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+9e:[ 	]+0f 24 0e 52 10 04[ 	]+fmsubss %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+a4:[ 	]+0f 24 0e 52 18 04[ 	]+fmsubss %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+aa:[ 	]+0f 24 0b d3 10[ 	]+fmsubsd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+af:[ 	]+0f 24 0b 52 10 04[ 	]+fmsubsd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+b5:[ 	]+0f 24 0b 52 18 04[ 	]+fmsubsd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+bb:[ 	]+0f 24 0f d3 10[ 	]+fmsubsd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+c0:[ 	]+0f 24 0f 52 10 04[ 	]+fmsubsd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+c6:[ 	]+0f 24 0f 52 18 04[ 	]+fmsubsd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+cc:[ 	]+0f 24 08 d3 10[ 	]+fmsubps %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+d1:[ 	]+0f 24 08 52 10 04[ 	]+fmsubps 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+d7:[ 	]+0f 24 08 52 18 04[ 	]+fmsubps %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+dd:[ 	]+0f 24 0c d3 10[ 	]+fmsubps %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+e2:[ 	]+0f 24 0c 52 10 04[ 	]+fmsubps %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+e8:[ 	]+0f 24 0c 52 18 04[ 	]+fmsubps %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+ee:[ 	]+0f 24 09 d3 10[ 	]+fmsubpd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+f3:[ 	]+0f 24 09 52 10 04[ 	]+fmsubpd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+f9:[ 	]+0f 24 09 52 18 04[ 	]+fmsubpd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+ff:[ 	]+0f 24 0d d3 10[ 	]+fmsubpd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+104:[ 	]+0f 24 0d 52 10 04[ 	]+fmsubpd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+10a:[ 	]+0f 24 0d 52 18 04[ 	]+fmsubpd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+110:[ 	]+0f 24 12 d3 10[ 	]+fnmaddss %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+115:[ 	]+0f 24 12 52 10 04[ 	]+fnmaddss 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+11b:[ 	]+0f 24 12 52 18 04[ 	]+fnmaddss %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+121:[ 	]+0f 24 16 d3 10[ 	]+fnmaddss %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+126:[ 	]+0f 24 16 52 10 04[ 	]+fnmaddss %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+12c:[ 	]+0f 24 16 52 18 04[ 	]+fnmaddss %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+132:[ 	]+0f 24 13 d3 10[ 	]+fnmaddsd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+137:[ 	]+0f 24 13 52 10 04[ 	]+fnmaddsd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+13d:[ 	]+0f 24 13 52 18 04[ 	]+fnmaddsd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+143:[ 	]+0f 24 17 d3 10[ 	]+fnmaddsd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+148:[ 	]+0f 24 17 52 10 04[ 	]+fnmaddsd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+14e:[ 	]+0f 24 17 52 18 04[ 	]+fnmaddsd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+154:[ 	]+0f 24 10 d3 10[ 	]+fnmaddps %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+159:[ 	]+0f 24 10 52 10 04[ 	]+fnmaddps 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+15f:[ 	]+0f 24 10 52 18 04[ 	]+fnmaddps %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+165:[ 	]+0f 24 14 d3 10[ 	]+fnmaddps %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+16a:[ 	]+0f 24 14 52 10 04[ 	]+fnmaddps %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+170:[ 	]+0f 24 14 52 18 04[ 	]+fnmaddps %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+176:[ 	]+0f 24 11 d3 10[ 	]+fnmaddpd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+17b:[ 	]+0f 24 11 52 10 04[ 	]+fnmaddpd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+181:[ 	]+0f 24 11 52 18 04[ 	]+fnmaddpd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+187:[ 	]+0f 24 15 d3 10[ 	]+fnmaddpd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+18c:[ 	]+0f 24 15 52 10 04[ 	]+fnmaddpd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+192:[ 	]+0f 24 15 52 18 04[ 	]+fnmaddpd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+198:[ 	]+0f 24 1a d3 10[ 	]+fnmsubss %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+19d:[ 	]+0f 24 1a 52 10 04[ 	]+fnmsubss 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+1a3:[ 	]+0f 24 1a 52 18 04[ 	]+fnmsubss %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+1a9:[ 	]+0f 24 1e d3 10[ 	]+fnmsubss %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+1ae:[ 	]+0f 24 1e 52 10 04[ 	]+fnmsubss %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1b4:[ 	]+0f 24 1e 52 18 04[ 	]+fnmsubss %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1ba:[ 	]+0f 24 1b d3 10[ 	]+fnmsubsd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+1bf:[ 	]+0f 24 1b 52 10 04[ 	]+fnmsubsd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+1c5:[ 	]+0f 24 1b 52 18 04[ 	]+fnmsubsd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+1cb:[ 	]+0f 24 1f d3 10[ 	]+fnmsubsd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+1d0:[ 	]+0f 24 1f 52 10 04[ 	]+fnmsubsd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1d6:[ 	]+0f 24 1f 52 18 04[ 	]+fnmsubsd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1dc:[ 	]+0f 24 18 d3 10[ 	]+fnmsubps %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+1e1:[ 	]+0f 24 18 52 10 04[ 	]+fnmsubps 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+1e7:[ 	]+0f 24 18 52 18 04[ 	]+fnmsubps %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+1ed:[ 	]+0f 24 1c d3 10[ 	]+fnmsubps %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+1f2:[ 	]+0f 24 1c 52 10 04[ 	]+fnmsubps %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f8:[ 	]+0f 24 1c 52 18 04[ 	]+fnmsubps %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1fe:[ 	]+0f 24 19 d3 10[ 	]+fnmsubpd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+203:[ 	]+0f 24 19 52 10 04[ 	]+fnmsubpd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+209:[ 	]+0f 24 19 52 18 04[ 	]+fnmsubpd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+20f:[ 	]+0f 24 1d d3 10[ 	]+fnmsubpd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+214:[ 	]+0f 24 1d 52 10 04[ 	]+fnmsubpd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+21a:[ 	]+0f 24 1d 52 18 04[ 	]+fnmsubpd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+220:[ 	]+0f 24 02 e5 b5[ 	]+fmaddss %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+225:[ 	]+0f 24 02 a7 b5 00 00 10 00[ 	]+fmaddss 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+22e:[ 	]+0f 24 02 a7 bd 00 00 10 00[ 	]+fmaddss %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+237:[ 	]+0f 24 06 e5 b5[ 	]+fmaddss %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+23c:[ 	]+0f 24 06 a7 b5 00 00 10 00[ 	]+fmaddss %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+245:[ 	]+0f 24 06 a7 bd 00 00 10 00[ 	]+fmaddss %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+24e:[ 	]+0f 24 03 e5 b5[ 	]+fmaddsd %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+253:[ 	]+0f 24 03 a7 b5 00 00 10 00[ 	]+fmaddsd 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+25c:[ 	]+0f 24 03 a7 bd 00 00 10 00[ 	]+fmaddsd %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+265:[ 	]+0f 24 07 e5 b5[ 	]+fmaddsd %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+26a:[ 	]+0f 24 07 a7 b5 00 00 10 00[ 	]+fmaddsd %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+273:[ 	]+0f 24 07 a7 bd 00 00 10 00[ 	]+fmaddsd %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+27c:[ 	]+0f 24 00 e5 b5[ 	]+fmaddps %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+281:[ 	]+0f 24 00 a7 b5 00 00 10 00[ 	]+fmaddps 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+28a:[ 	]+0f 24 00 a7 bd 00 00 10 00[ 	]+fmaddps %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+293:[ 	]+0f 24 04 e5 b5[ 	]+fmaddps %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+298:[ 	]+0f 24 04 a7 b5 00 00 10 00[ 	]+fmaddps %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2a1:[ 	]+0f 24 04 a7 bd 00 00 10 00[ 	]+fmaddps %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+2aa:[ 	]+0f 24 01 e5 b5[ 	]+fmaddpd %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+2af:[ 	]+0f 24 01 a7 b5 00 00 10 00[ 	]+fmaddpd 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+2b8:[ 	]+0f 24 01 a7 bd 00 00 10 00[ 	]+fmaddpd %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+2c1:[ 	]+0f 24 05 e5 b5[ 	]+fmaddpd %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+2c6:[ 	]+0f 24 05 a7 b5 00 00 10 00[ 	]+fmaddpd %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2cf:[ 	]+0f 24 05 a7 bd 00 00 10 00[ 	]+fmaddpd %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+2d8:[ 	]+0f 24 0a e5 b5[ 	]+fmsubss %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+2dd:[ 	]+0f 24 0a a7 b5 00 00 10 00[ 	]+fmsubss 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+2e6:[ 	]+0f 24 0a a7 bd 00 00 10 00[ 	]+fmsubss %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+2ef:[ 	]+0f 24 0e e5 b5[ 	]+fmsubss %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+2f4:[ 	]+0f 24 0e a7 b5 00 00 10 00[ 	]+fmsubss %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2fd:[ 	]+0f 24 0e a7 bd 00 00 10 00[ 	]+fmsubss %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+306:[ 	]+0f 24 0b e5 b5[ 	]+fmsubsd %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+30b:[ 	]+0f 24 0b a7 b5 00 00 10 00[ 	]+fmsubsd 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+314:[ 	]+0f 24 0b a7 bd 00 00 10 00[ 	]+fmsubsd %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+31d:[ 	]+0f 24 0f e5 b5[ 	]+fmsubsd %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+322:[ 	]+0f 24 0f a7 b5 00 00 10 00[ 	]+fmsubsd %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+32b:[ 	]+0f 24 0f a7 bd 00 00 10 00[ 	]+fmsubsd %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+334:[ 	]+0f 24 08 e5 b5[ 	]+fmsubps %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+339:[ 	]+0f 24 08 a7 b5 00 00 10 00[ 	]+fmsubps 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+342:[ 	]+0f 24 08 a7 bd 00 00 10 00[ 	]+fmsubps %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+34b:[ 	]+0f 24 0c e5 b5[ 	]+fmsubps %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+350:[ 	]+0f 24 0c a7 b5 00 00 10 00[ 	]+fmsubps %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+359:[ 	]+0f 24 0c a7 bd 00 00 10 00[ 	]+fmsubps %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+362:[ 	]+0f 24 09 e5 b5[ 	]+fmsubpd %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+367:[ 	]+0f 24 09 a7 b5 00 00 10 00[ 	]+fmsubpd 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+370:[ 	]+0f 24 09 a7 bd 00 00 10 00[ 	]+fmsubpd %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+379:[ 	]+0f 24 0d e5 b5[ 	]+fmsubpd %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+37e:[ 	]+0f 24 0d a7 b5 00 00 10 00[ 	]+fmsubpd %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+387:[ 	]+0f 24 0d a7 bd 00 00 10 00[ 	]+fmsubpd %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+390:[ 	]+0f 24 12 e5 b5[ 	]+fnmaddss %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+395:[ 	]+0f 24 12 a7 b5 00 00 10 00[ 	]+fnmaddss 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+39e:[ 	]+0f 24 12 a7 bd 00 00 10 00[ 	]+fnmaddss %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+3a7:[ 	]+0f 24 16 e5 b5[ 	]+fnmaddss %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+3ac:[ 	]+0f 24 16 a7 b5 00 00 10 00[ 	]+fnmaddss %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+3b5:[ 	]+0f 24 16 a7 bd 00 00 10 00[ 	]+fnmaddss %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+3be:[ 	]+0f 24 13 e5 b5[ 	]+fnmaddsd %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+3c3:[ 	]+0f 24 13 a7 b5 00 00 10 00[ 	]+fnmaddsd 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+3cc:[ 	]+0f 24 13 a7 bd 00 00 10 00[ 	]+fnmaddsd %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+3d5:[ 	]+0f 24 17 e5 b5[ 	]+fnmaddsd %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+3da:[ 	]+0f 24 17 a7 b5 00 00 10 00[ 	]+fnmaddsd %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+3e3:[ 	]+0f 24 17 a7 bd 00 00 10 00[ 	]+fnmaddsd %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+3ec:[ 	]+0f 24 10 e5 b5[ 	]+fnmaddps %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+3f1:[ 	]+0f 24 10 a7 b5 00 00 10 00[ 	]+fnmaddps 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+3fa:[ 	]+0f 24 10 a7 bd 00 00 10 00[ 	]+fnmaddps %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+403:[ 	]+0f 24 14 e5 b5[ 	]+fnmaddps %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+408:[ 	]+0f 24 14 a7 b5 00 00 10 00[ 	]+fnmaddps %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+411:[ 	]+0f 24 14 a7 bd 00 00 10 00[ 	]+fnmaddps %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+41a:[ 	]+0f 24 11 e5 b5[ 	]+fnmaddpd %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+41f:[ 	]+0f 24 11 a7 b5 00 00 10 00[ 	]+fnmaddpd 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+428:[ 	]+0f 24 11 a7 bd 00 00 10 00[ 	]+fnmaddpd %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+431:[ 	]+0f 24 15 e5 b5[ 	]+fnmaddpd %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+436:[ 	]+0f 24 15 a7 b5 00 00 10 00[ 	]+fnmaddpd %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+43f:[ 	]+0f 24 15 a7 bd 00 00 10 00[ 	]+fnmaddpd %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+448:[ 	]+0f 24 1a e5 b5[ 	]+fnmsubss %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+44d:[ 	]+0f 24 1a a7 b5 00 00 10 00[ 	]+fnmsubss 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+456:[ 	]+0f 24 1a a7 bd 00 00 10 00[ 	]+fnmsubss %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+45f:[ 	]+0f 24 1e e5 b5[ 	]+fnmsubss %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+464:[ 	]+0f 24 1e a7 b5 00 00 10 00[ 	]+fnmsubss %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+46d:[ 	]+0f 24 1e a7 bd 00 00 10 00[ 	]+fnmsubss %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+476:[ 	]+0f 24 1b e5 b5[ 	]+fnmsubsd %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+47b:[ 	]+0f 24 1b a7 b5 00 00 10 00[ 	]+fnmsubsd 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+484:[ 	]+0f 24 1b a7 bd 00 00 10 00[ 	]+fnmsubsd %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+48d:[ 	]+0f 24 1f e5 b5[ 	]+fnmsubsd %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+492:[ 	]+0f 24 1f a7 b5 00 00 10 00[ 	]+fnmsubsd %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+49b:[ 	]+0f 24 1f a7 bd 00 00 10 00[ 	]+fnmsubsd %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+4a4:[ 	]+0f 24 18 e5 b5[ 	]+fnmsubps %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+4a9:[ 	]+0f 24 18 a7 b5 00 00 10 00[ 	]+fnmsubps 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+4b2:[ 	]+0f 24 18 a7 bd 00 00 10 00[ 	]+fnmsubps %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+4bb:[ 	]+0f 24 1c e5 b5[ 	]+fnmsubps %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+4c0:[ 	]+0f 24 1c a7 b5 00 00 10 00[ 	]+fnmsubps %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+4c9:[ 	]+0f 24 1c a7 bd 00 00 10 00[ 	]+fnmsubps %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+4d2:[ 	]+0f 24 19 e5 b5[ 	]+fnmsubpd %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+4d7:[ 	]+0f 24 19 a7 b5 00 00 10 00[ 	]+fnmsubpd 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+4e0:[ 	]+0f 24 19 a7 bd 00 00 10 00[ 	]+fnmsubpd %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+4e9:[ 	]+0f 24 1d e5 b5[ 	]+fnmsubpd %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+4ee:[ 	]+0f 24 1d a7 b5 00 00 10 00[ 	]+fnmsubpd %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+4f7:[ 	]+0f 24 1d a7 bd 00 00 10 00[ 	]+fnmsubpd %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+500:[ 	]+0f 24 02 e3 14[ 	]+fmaddss %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+505:[ 	]+0f 24 02 62 14 04[ 	]+fmaddss 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+50b:[ 	]+0f 24 02 62 1c 04[ 	]+fmaddss %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+511:[ 	]+0f 24 06 e3 14[ 	]+fmaddss %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+516:[ 	]+0f 24 06 62 14 04[ 	]+fmaddss %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+51c:[ 	]+0f 24 06 62 1c 04[ 	]+fmaddss %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+522:[ 	]+0f 24 03 e3 14[ 	]+fmaddsd %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+527:[ 	]+0f 24 03 62 14 04[ 	]+fmaddsd 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+52d:[ 	]+0f 24 03 62 1c 04[ 	]+fmaddsd %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+533:[ 	]+0f 24 07 e3 14[ 	]+fmaddsd %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+538:[ 	]+0f 24 07 62 14 04[ 	]+fmaddsd %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+53e:[ 	]+0f 24 07 62 1c 04[ 	]+fmaddsd %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+544:[ 	]+0f 24 00 e3 14[ 	]+fmaddps %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+549:[ 	]+0f 24 00 62 14 04[ 	]+fmaddps 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+54f:[ 	]+0f 24 00 62 1c 04[ 	]+fmaddps %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+555:[ 	]+0f 24 04 e3 14[ 	]+fmaddps %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+55a:[ 	]+0f 24 04 62 14 04[ 	]+fmaddps %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+560:[ 	]+0f 24 04 62 1c 04[ 	]+fmaddps %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+566:[ 	]+0f 24 01 e3 14[ 	]+fmaddpd %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+56b:[ 	]+0f 24 01 62 14 04[ 	]+fmaddpd 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+571:[ 	]+0f 24 01 62 1c 04[ 	]+fmaddpd %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+577:[ 	]+0f 24 05 e3 14[ 	]+fmaddpd %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+57c:[ 	]+0f 24 05 62 14 04[ 	]+fmaddpd %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+582:[ 	]+0f 24 05 62 1c 04[ 	]+fmaddpd %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+588:[ 	]+0f 24 0a e3 14[ 	]+fmsubss %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+58d:[ 	]+0f 24 0a 62 14 04[ 	]+fmsubss 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+593:[ 	]+0f 24 0a 62 1c 04[ 	]+fmsubss %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+599:[ 	]+0f 24 0e e3 14[ 	]+fmsubss %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+59e:[ 	]+0f 24 0e 62 14 04[ 	]+fmsubss %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+5a4:[ 	]+0f 24 0e 62 1c 04[ 	]+fmsubss %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+5aa:[ 	]+0f 24 0b e3 14[ 	]+fmsubsd %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+5af:[ 	]+0f 24 0b 62 14 04[ 	]+fmsubsd 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+5b5:[ 	]+0f 24 0b 62 1c 04[ 	]+fmsubsd %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+5bb:[ 	]+0f 24 0f e3 14[ 	]+fmsubsd %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+5c0:[ 	]+0f 24 0f 62 14 04[ 	]+fmsubsd %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+5c6:[ 	]+0f 24 0f 62 1c 04[ 	]+fmsubsd %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+5cc:[ 	]+0f 24 08 e3 14[ 	]+fmsubps %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+5d1:[ 	]+0f 24 08 62 14 04[ 	]+fmsubps 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+5d7:[ 	]+0f 24 08 62 1c 04[ 	]+fmsubps %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+5dd:[ 	]+0f 24 0c e3 14[ 	]+fmsubps %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+5e2:[ 	]+0f 24 0c 62 14 04[ 	]+fmsubps %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+5e8:[ 	]+0f 24 0c 62 1c 04[ 	]+fmsubps %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+5ee:[ 	]+0f 24 09 e3 14[ 	]+fmsubpd %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+5f3:[ 	]+0f 24 09 62 14 04[ 	]+fmsubpd 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+5f9:[ 	]+0f 24 09 62 1c 04[ 	]+fmsubpd %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+5ff:[ 	]+0f 24 0d e3 14[ 	]+fmsubpd %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+604:[ 	]+0f 24 0d 62 14 04[ 	]+fmsubpd %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+60a:[ 	]+0f 24 0d 62 1c 04[ 	]+fmsubpd %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+610:[ 	]+0f 24 12 e3 14[ 	]+fnmaddss %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+615:[ 	]+0f 24 12 62 14 04[ 	]+fnmaddss 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+61b:[ 	]+0f 24 12 62 1c 04[ 	]+fnmaddss %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+621:[ 	]+0f 24 16 e3 14[ 	]+fnmaddss %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+626:[ 	]+0f 24 16 62 14 04[ 	]+fnmaddss %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+62c:[ 	]+0f 24 16 62 1c 04[ 	]+fnmaddss %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+632:[ 	]+0f 24 13 e3 14[ 	]+fnmaddsd %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+637:[ 	]+0f 24 13 62 14 04[ 	]+fnmaddsd 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+63d:[ 	]+0f 24 13 62 1c 04[ 	]+fnmaddsd %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+643:[ 	]+0f 24 17 e3 14[ 	]+fnmaddsd %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+648:[ 	]+0f 24 17 62 14 04[ 	]+fnmaddsd %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+64e:[ 	]+0f 24 17 62 1c 04[ 	]+fnmaddsd %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+654:[ 	]+0f 24 10 e3 14[ 	]+fnmaddps %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+659:[ 	]+0f 24 10 62 14 04[ 	]+fnmaddps 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+65f:[ 	]+0f 24 10 62 1c 04[ 	]+fnmaddps %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+665:[ 	]+0f 24 14 e3 14[ 	]+fnmaddps %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+66a:[ 	]+0f 24 14 62 14 04[ 	]+fnmaddps %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+670:[ 	]+0f 24 14 62 1c 04[ 	]+fnmaddps %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+676:[ 	]+0f 24 11 e3 14[ 	]+fnmaddpd %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+67b:[ 	]+0f 24 11 62 14 04[ 	]+fnmaddpd 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+681:[ 	]+0f 24 11 62 1c 04[ 	]+fnmaddpd %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+687:[ 	]+0f 24 15 e3 14[ 	]+fnmaddpd %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+68c:[ 	]+0f 24 15 62 14 04[ 	]+fnmaddpd %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+692:[ 	]+0f 24 15 62 1c 04[ 	]+fnmaddpd %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+698:[ 	]+0f 24 1a e3 14[ 	]+fnmsubss %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+69d:[ 	]+0f 24 1a 62 14 04[ 	]+fnmsubss 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+6a3:[ 	]+0f 24 1a 62 1c 04[ 	]+fnmsubss %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+6a9:[ 	]+0f 24 1e e3 14[ 	]+fnmsubss %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+6ae:[ 	]+0f 24 1e 62 14 04[ 	]+fnmsubss %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+6b4:[ 	]+0f 24 1e 62 1c 04[ 	]+fnmsubss %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+6ba:[ 	]+0f 24 1b e3 14[ 	]+fnmsubsd %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+6bf:[ 	]+0f 24 1b 62 14 04[ 	]+fnmsubsd 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+6c5:[ 	]+0f 24 1b 62 1c 04[ 	]+fnmsubsd %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+6cb:[ 	]+0f 24 1f e3 14[ 	]+fnmsubsd %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+6d0:[ 	]+0f 24 1f 62 14 04[ 	]+fnmsubsd %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+6d6:[ 	]+0f 24 1f 62 1c 04[ 	]+fnmsubsd %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+6dc:[ 	]+0f 24 18 e3 14[ 	]+fnmsubps %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+6e1:[ 	]+0f 24 18 62 14 04[ 	]+fnmsubps 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+6e7:[ 	]+0f 24 18 62 1c 04[ 	]+fnmsubps %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+6ed:[ 	]+0f 24 1c e3 14[ 	]+fnmsubps %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+6f2:[ 	]+0f 24 1c 62 14 04[ 	]+fnmsubps %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+6f8:[ 	]+0f 24 1c 62 1c 04[ 	]+fnmsubps %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+6fe:[ 	]+0f 24 19 e3 14[ 	]+fnmsubpd %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+703:[ 	]+0f 24 19 62 14 04[ 	]+fnmsubpd 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+709:[ 	]+0f 24 19 62 1c 04[ 	]+fnmsubpd %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+70f:[ 	]+0f 24 1d e3 14[ 	]+fnmsubpd %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+714:[ 	]+0f 24 1d 62 14 04[ 	]+fnmsubpd %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+71a:[ 	]+0f 24 1d 62 1c 04[ 	]+fnmsubpd %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+720:[ 	]+0f 24 02 d3 b0[ 	]+fmaddss %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+725:[ 	]+0f 24 02 52 b0 04[ 	]+fmaddss 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+72b:[ 	]+0f 24 02 52 b8 04[ 	]+fmaddss %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+731:[ 	]+0f 24 06 d3 b0[ 	]+fmaddss %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+736:[ 	]+0f 24 06 52 b0 04[ 	]+fmaddss %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+73c:[ 	]+0f 24 06 52 b8 04[ 	]+fmaddss %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+742:[ 	]+0f 24 03 d3 b0[ 	]+fmaddsd %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+747:[ 	]+0f 24 03 52 b0 04[ 	]+fmaddsd 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+74d:[ 	]+0f 24 03 52 b8 04[ 	]+fmaddsd %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+753:[ 	]+0f 24 07 d3 b0[ 	]+fmaddsd %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+758:[ 	]+0f 24 07 52 b0 04[ 	]+fmaddsd %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+75e:[ 	]+0f 24 07 52 b8 04[ 	]+fmaddsd %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+764:[ 	]+0f 24 00 d3 b0[ 	]+fmaddps %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+769:[ 	]+0f 24 00 52 b0 04[ 	]+fmaddps 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+76f:[ 	]+0f 24 00 52 b8 04[ 	]+fmaddps %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+775:[ 	]+0f 24 04 d3 b0[ 	]+fmaddps %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+77a:[ 	]+0f 24 04 52 b0 04[ 	]+fmaddps %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+780:[ 	]+0f 24 04 52 b8 04[ 	]+fmaddps %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+786:[ 	]+0f 24 01 d3 b0[ 	]+fmaddpd %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+78b:[ 	]+0f 24 01 52 b0 04[ 	]+fmaddpd 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+791:[ 	]+0f 24 01 52 b8 04[ 	]+fmaddpd %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+797:[ 	]+0f 24 05 d3 b0[ 	]+fmaddpd %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+79c:[ 	]+0f 24 05 52 b0 04[ 	]+fmaddpd %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+7a2:[ 	]+0f 24 05 52 b8 04[ 	]+fmaddpd %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+7a8:[ 	]+0f 24 0a d3 b0[ 	]+fmsubss %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+7ad:[ 	]+0f 24 0a 52 b0 04[ 	]+fmsubss 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+7b3:[ 	]+0f 24 0a 52 b8 04[ 	]+fmsubss %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+7b9:[ 	]+0f 24 0e d3 b0[ 	]+fmsubss %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+7be:[ 	]+0f 24 0e 52 b0 04[ 	]+fmsubss %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+7c4:[ 	]+0f 24 0e 52 b8 04[ 	]+fmsubss %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+7ca:[ 	]+0f 24 0b d3 b0[ 	]+fmsubsd %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+7cf:[ 	]+0f 24 0b 52 b0 04[ 	]+fmsubsd 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+7d5:[ 	]+0f 24 0b 52 b8 04[ 	]+fmsubsd %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+7db:[ 	]+0f 24 0f d3 b0[ 	]+fmsubsd %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+7e0:[ 	]+0f 24 0f 52 b0 04[ 	]+fmsubsd %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+7e6:[ 	]+0f 24 0f 52 b8 04[ 	]+fmsubsd %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+7ec:[ 	]+0f 24 08 d3 b0[ 	]+fmsubps %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+7f1:[ 	]+0f 24 08 52 b0 04[ 	]+fmsubps 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+7f7:[ 	]+0f 24 08 52 b8 04[ 	]+fmsubps %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+7fd:[ 	]+0f 24 0c d3 b0[ 	]+fmsubps %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+802:[ 	]+0f 24 0c 52 b0 04[ 	]+fmsubps %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+808:[ 	]+0f 24 0c 52 b8 04[ 	]+fmsubps %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+80e:[ 	]+0f 24 09 d3 b0[ 	]+fmsubpd %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+813:[ 	]+0f 24 09 52 b0 04[ 	]+fmsubpd 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+819:[ 	]+0f 24 09 52 b8 04[ 	]+fmsubpd %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+81f:[ 	]+0f 24 0d d3 b0[ 	]+fmsubpd %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+824:[ 	]+0f 24 0d 52 b0 04[ 	]+fmsubpd %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+82a:[ 	]+0f 24 0d 52 b8 04[ 	]+fmsubpd %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+830:[ 	]+0f 24 12 d3 b0[ 	]+fnmaddss %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+835:[ 	]+0f 24 12 52 b0 04[ 	]+fnmaddss 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+83b:[ 	]+0f 24 12 52 b8 04[ 	]+fnmaddss %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+841:[ 	]+0f 24 16 d3 b0[ 	]+fnmaddss %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+846:[ 	]+0f 24 16 52 b0 04[ 	]+fnmaddss %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+84c:[ 	]+0f 24 16 52 b8 04[ 	]+fnmaddss %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+852:[ 	]+0f 24 13 d3 b0[ 	]+fnmaddsd %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+857:[ 	]+0f 24 13 52 b0 04[ 	]+fnmaddsd 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+85d:[ 	]+0f 24 13 52 b8 04[ 	]+fnmaddsd %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+863:[ 	]+0f 24 17 d3 b0[ 	]+fnmaddsd %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+868:[ 	]+0f 24 17 52 b0 04[ 	]+fnmaddsd %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+86e:[ 	]+0f 24 17 52 b8 04[ 	]+fnmaddsd %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+874:[ 	]+0f 24 10 d3 b0[ 	]+fnmaddps %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+879:[ 	]+0f 24 10 52 b0 04[ 	]+fnmaddps 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+87f:[ 	]+0f 24 10 52 b8 04[ 	]+fnmaddps %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+885:[ 	]+0f 24 14 d3 b0[ 	]+fnmaddps %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+88a:[ 	]+0f 24 14 52 b0 04[ 	]+fnmaddps %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+890:[ 	]+0f 24 14 52 b8 04[ 	]+fnmaddps %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+896:[ 	]+0f 24 11 d3 b0[ 	]+fnmaddpd %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+89b:[ 	]+0f 24 11 52 b0 04[ 	]+fnmaddpd 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+8a1:[ 	]+0f 24 11 52 b8 04[ 	]+fnmaddpd %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+8a7:[ 	]+0f 24 15 d3 b0[ 	]+fnmaddpd %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+8ac:[ 	]+0f 24 15 52 b0 04[ 	]+fnmaddpd %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+8b2:[ 	]+0f 24 15 52 b8 04[ 	]+fnmaddpd %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+8b8:[ 	]+0f 24 1a d3 b0[ 	]+fnmsubss %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+8bd:[ 	]+0f 24 1a 52 b0 04[ 	]+fnmsubss 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+8c3:[ 	]+0f 24 1a 52 b8 04[ 	]+fnmsubss %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+8c9:[ 	]+0f 24 1e d3 b0[ 	]+fnmsubss %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+8ce:[ 	]+0f 24 1e 52 b0 04[ 	]+fnmsubss %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+8d4:[ 	]+0f 24 1e 52 b8 04[ 	]+fnmsubss %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+8da:[ 	]+0f 24 1b d3 b0[ 	]+fnmsubsd %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+8df:[ 	]+0f 24 1b 52 b0 04[ 	]+fnmsubsd 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+8e5:[ 	]+0f 24 1b 52 b8 04[ 	]+fnmsubsd %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+8eb:[ 	]+0f 24 1f d3 b0[ 	]+fnmsubsd %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+8f0:[ 	]+0f 24 1f 52 b0 04[ 	]+fnmsubsd %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+8f6:[ 	]+0f 24 1f 52 b8 04[ 	]+fnmsubsd %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+8fc:[ 	]+0f 24 18 d3 b0[ 	]+fnmsubps %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+901:[ 	]+0f 24 18 52 b0 04[ 	]+fnmsubps 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+907:[ 	]+0f 24 18 52 b8 04[ 	]+fnmsubps %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+90d:[ 	]+0f 24 1c d3 b0[ 	]+fnmsubps %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+912:[ 	]+0f 24 1c 52 b0 04[ 	]+fnmsubps %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+918:[ 	]+0f 24 1c 52 b8 04[ 	]+fnmsubps %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+91e:[ 	]+0f 24 19 d3 b0[ 	]+fnmsubpd %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+923:[ 	]+0f 24 19 52 b0 04[ 	]+fnmsubpd 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+929:[ 	]+0f 24 19 52 b8 04[ 	]+fnmsubpd %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+92f:[ 	]+0f 24 1d d3 b0[ 	]+fnmsubpd %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+934:[ 	]+0f 24 1d 52 b0 04[ 	]+fnmsubpd %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+93a:[ 	]+0f 24 1d 52 b8 04[ 	]+fnmsubpd %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+940:[ 	]+0f 24 02 d5 11[ 	]+fmaddss %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+945:[ 	]+0f 24 02 52 10 04[ 	]+fmaddss 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+94b:[ 	]+0f 24 02 52 18 04[ 	]+fmaddss %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+951:[ 	]+0f 24 06 d5 11[ 	]+fmaddss %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+956:[ 	]+0f 24 06 52 10 04[ 	]+fmaddss %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+95c:[ 	]+0f 24 06 52 18 04[ 	]+fmaddss %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+962:[ 	]+0f 24 03 d5 11[ 	]+fmaddsd %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+967:[ 	]+0f 24 03 52 10 04[ 	]+fmaddsd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+96d:[ 	]+0f 24 03 52 18 04[ 	]+fmaddsd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+973:[ 	]+0f 24 07 d5 11[ 	]+fmaddsd %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+978:[ 	]+0f 24 07 52 10 04[ 	]+fmaddsd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+97e:[ 	]+0f 24 07 52 18 04[ 	]+fmaddsd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+984:[ 	]+0f 24 00 d5 11[ 	]+fmaddps %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+989:[ 	]+0f 24 00 52 10 04[ 	]+fmaddps 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+98f:[ 	]+0f 24 00 52 18 04[ 	]+fmaddps %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+995:[ 	]+0f 24 04 d5 11[ 	]+fmaddps %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+99a:[ 	]+0f 24 04 52 10 04[ 	]+fmaddps %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+9a0:[ 	]+0f 24 04 52 18 04[ 	]+fmaddps %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+9a6:[ 	]+0f 24 01 d5 11[ 	]+fmaddpd %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+9ab:[ 	]+0f 24 01 52 10 04[ 	]+fmaddpd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+9b1:[ 	]+0f 24 01 52 18 04[ 	]+fmaddpd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+9b7:[ 	]+0f 24 05 d5 11[ 	]+fmaddpd %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+9bc:[ 	]+0f 24 05 52 10 04[ 	]+fmaddpd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+9c2:[ 	]+0f 24 05 52 18 04[ 	]+fmaddpd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+9c8:[ 	]+0f 24 0a d5 11[ 	]+fmsubss %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+9cd:[ 	]+0f 24 0a 52 10 04[ 	]+fmsubss 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+9d3:[ 	]+0f 24 0a 52 18 04[ 	]+fmsubss %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+9d9:[ 	]+0f 24 0e d5 11[ 	]+fmsubss %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+9de:[ 	]+0f 24 0e 52 10 04[ 	]+fmsubss %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+9e4:[ 	]+0f 24 0e 52 18 04[ 	]+fmsubss %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+9ea:[ 	]+0f 24 0b d5 11[ 	]+fmsubsd %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+9ef:[ 	]+0f 24 0b 52 10 04[ 	]+fmsubsd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+9f5:[ 	]+0f 24 0b 52 18 04[ 	]+fmsubsd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+9fb:[ 	]+0f 24 0f d5 11[ 	]+fmsubsd %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+a00:[ 	]+0f 24 0f 52 10 04[ 	]+fmsubsd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+a06:[ 	]+0f 24 0f 52 18 04[ 	]+fmsubsd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+a0c:[ 	]+0f 24 08 d5 11[ 	]+fmsubps %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+a11:[ 	]+0f 24 08 52 10 04[ 	]+fmsubps 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+a17:[ 	]+0f 24 08 52 18 04[ 	]+fmsubps %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+a1d:[ 	]+0f 24 0c d5 11[ 	]+fmsubps %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+a22:[ 	]+0f 24 0c 52 10 04[ 	]+fmsubps %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+a28:[ 	]+0f 24 0c 52 18 04[ 	]+fmsubps %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+a2e:[ 	]+0f 24 09 d5 11[ 	]+fmsubpd %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+a33:[ 	]+0f 24 09 52 10 04[ 	]+fmsubpd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+a39:[ 	]+0f 24 09 52 18 04[ 	]+fmsubpd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+a3f:[ 	]+0f 24 0d d5 11[ 	]+fmsubpd %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+a44:[ 	]+0f 24 0d 52 10 04[ 	]+fmsubpd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+a4a:[ 	]+0f 24 0d 52 18 04[ 	]+fmsubpd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+a50:[ 	]+0f 24 12 d5 11[ 	]+fnmaddss %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+a55:[ 	]+0f 24 12 52 10 04[ 	]+fnmaddss 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+a5b:[ 	]+0f 24 12 52 18 04[ 	]+fnmaddss %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+a61:[ 	]+0f 24 16 d5 11[ 	]+fnmaddss %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+a66:[ 	]+0f 24 16 52 10 04[ 	]+fnmaddss %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+a6c:[ 	]+0f 24 16 52 18 04[ 	]+fnmaddss %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+a72:[ 	]+0f 24 13 d5 11[ 	]+fnmaddsd %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+a77:[ 	]+0f 24 13 52 10 04[ 	]+fnmaddsd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+a7d:[ 	]+0f 24 13 52 18 04[ 	]+fnmaddsd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+a83:[ 	]+0f 24 17 d5 11[ 	]+fnmaddsd %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+a88:[ 	]+0f 24 17 52 10 04[ 	]+fnmaddsd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+a8e:[ 	]+0f 24 17 52 18 04[ 	]+fnmaddsd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+a94:[ 	]+0f 24 10 d5 11[ 	]+fnmaddps %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+a99:[ 	]+0f 24 10 52 10 04[ 	]+fnmaddps 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+a9f:[ 	]+0f 24 10 52 18 04[ 	]+fnmaddps %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+aa5:[ 	]+0f 24 14 d5 11[ 	]+fnmaddps %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+aaa:[ 	]+0f 24 14 52 10 04[ 	]+fnmaddps %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+ab0:[ 	]+0f 24 14 52 18 04[ 	]+fnmaddps %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+ab6:[ 	]+0f 24 11 d5 11[ 	]+fnmaddpd %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+abb:[ 	]+0f 24 11 52 10 04[ 	]+fnmaddpd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+ac1:[ 	]+0f 24 11 52 18 04[ 	]+fnmaddpd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+ac7:[ 	]+0f 24 15 d5 11[ 	]+fnmaddpd %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+acc:[ 	]+0f 24 15 52 10 04[ 	]+fnmaddpd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+ad2:[ 	]+0f 24 15 52 18 04[ 	]+fnmaddpd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+ad8:[ 	]+0f 24 1a d5 11[ 	]+fnmsubss %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+add:[ 	]+0f 24 1a 52 10 04[ 	]+fnmsubss 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+ae3:[ 	]+0f 24 1a 52 18 04[ 	]+fnmsubss %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+ae9:[ 	]+0f 24 1e d5 11[ 	]+fnmsubss %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+aee:[ 	]+0f 24 1e 52 10 04[ 	]+fnmsubss %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+af4:[ 	]+0f 24 1e 52 18 04[ 	]+fnmsubss %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+afa:[ 	]+0f 24 1b d5 11[ 	]+fnmsubsd %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+aff:[ 	]+0f 24 1b 52 10 04[ 	]+fnmsubsd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+b05:[ 	]+0f 24 1b 52 18 04[ 	]+fnmsubsd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+b0b:[ 	]+0f 24 1f d5 11[ 	]+fnmsubsd %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+b10:[ 	]+0f 24 1f 52 10 04[ 	]+fnmsubsd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+b16:[ 	]+0f 24 1f 52 18 04[ 	]+fnmsubsd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+b1c:[ 	]+0f 24 18 d5 11[ 	]+fnmsubps %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+b21:[ 	]+0f 24 18 52 10 04[ 	]+fnmsubps 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+b27:[ 	]+0f 24 18 52 18 04[ 	]+fnmsubps %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+b2d:[ 	]+0f 24 1c d5 11[ 	]+fnmsubps %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+b32:[ 	]+0f 24 1c 52 10 04[ 	]+fnmsubps %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+b38:[ 	]+0f 24 1c 52 18 04[ 	]+fnmsubps %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+b3e:[ 	]+0f 24 19 d5 11[ 	]+fnmsubpd %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+b43:[ 	]+0f 24 19 52 10 04[ 	]+fnmsubpd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+b49:[ 	]+0f 24 19 52 18 04[ 	]+fnmsubpd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+b4f:[ 	]+0f 24 1d d5 11[ 	]+fnmsubpd %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+b54:[ 	]+0f 24 1d 52 10 04[ 	]+fnmsubpd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+b5a:[ 	]+0f 24 1d 52 18 04[ 	]+fnmsubpd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+b60:[ 	]+0f 24 02 d3 10[ 	]+fmaddss %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+b65:[ 	]+0f 24 02 97 11 00 00 10 00[ 	]+fmaddss 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+b6e:[ 	]+0f 24 02 97 19 00 00 10 00[ 	]+fmaddss %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+b77:[ 	]+0f 24 06 d3 10[ 	]+fmaddss %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+b7c:[ 	]+0f 24 06 97 11 00 00 10 00[ 	]+fmaddss %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+b85:[ 	]+0f 24 06 97 19 00 00 10 00[ 	]+fmaddss %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+b8e:[ 	]+0f 24 03 d3 10[ 	]+fmaddsd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+b93:[ 	]+0f 24 03 97 11 00 00 10 00[ 	]+fmaddsd 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+b9c:[ 	]+0f 24 03 97 19 00 00 10 00[ 	]+fmaddsd %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+ba5:[ 	]+0f 24 07 d3 10[ 	]+fmaddsd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+baa:[ 	]+0f 24 07 97 11 00 00 10 00[ 	]+fmaddsd %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+bb3:[ 	]+0f 24 07 97 19 00 00 10 00[ 	]+fmaddsd %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+bbc:[ 	]+0f 24 00 d3 10[ 	]+fmaddps %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+bc1:[ 	]+0f 24 00 97 11 00 00 10 00[ 	]+fmaddps 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+bca:[ 	]+0f 24 00 97 19 00 00 10 00[ 	]+fmaddps %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+bd3:[ 	]+0f 24 04 d3 10[ 	]+fmaddps %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+bd8:[ 	]+0f 24 04 97 11 00 00 10 00[ 	]+fmaddps %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+be1:[ 	]+0f 24 04 97 19 00 00 10 00[ 	]+fmaddps %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+bea:[ 	]+0f 24 01 d3 10[ 	]+fmaddpd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+bef:[ 	]+0f 24 01 97 11 00 00 10 00[ 	]+fmaddpd 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+bf8:[ 	]+0f 24 01 97 19 00 00 10 00[ 	]+fmaddpd %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+c01:[ 	]+0f 24 05 d3 10[ 	]+fmaddpd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+c06:[ 	]+0f 24 05 97 11 00 00 10 00[ 	]+fmaddpd %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+c0f:[ 	]+0f 24 05 97 19 00 00 10 00[ 	]+fmaddpd %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+c18:[ 	]+0f 24 0a d3 10[ 	]+fmsubss %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+c1d:[ 	]+0f 24 0a 97 11 00 00 10 00[ 	]+fmsubss 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+c26:[ 	]+0f 24 0a 97 19 00 00 10 00[ 	]+fmsubss %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+c2f:[ 	]+0f 24 0e d3 10[ 	]+fmsubss %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+c34:[ 	]+0f 24 0e 97 11 00 00 10 00[ 	]+fmsubss %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+c3d:[ 	]+0f 24 0e 97 19 00 00 10 00[ 	]+fmsubss %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+c46:[ 	]+0f 24 0b d3 10[ 	]+fmsubsd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+c4b:[ 	]+0f 24 0b 97 11 00 00 10 00[ 	]+fmsubsd 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+c54:[ 	]+0f 24 0b 97 19 00 00 10 00[ 	]+fmsubsd %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+c5d:[ 	]+0f 24 0f d3 10[ 	]+fmsubsd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+c62:[ 	]+0f 24 0f 97 11 00 00 10 00[ 	]+fmsubsd %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+c6b:[ 	]+0f 24 0f 97 19 00 00 10 00[ 	]+fmsubsd %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+c74:[ 	]+0f 24 08 d3 10[ 	]+fmsubps %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+c79:[ 	]+0f 24 08 97 11 00 00 10 00[ 	]+fmsubps 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+c82:[ 	]+0f 24 08 97 19 00 00 10 00[ 	]+fmsubps %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+c8b:[ 	]+0f 24 0c d3 10[ 	]+fmsubps %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+c90:[ 	]+0f 24 0c 97 11 00 00 10 00[ 	]+fmsubps %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+c99:[ 	]+0f 24 0c 97 19 00 00 10 00[ 	]+fmsubps %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+ca2:[ 	]+0f 24 09 d3 10[ 	]+fmsubpd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+ca7:[ 	]+0f 24 09 97 11 00 00 10 00[ 	]+fmsubpd 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+cb0:[ 	]+0f 24 09 97 19 00 00 10 00[ 	]+fmsubpd %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+cb9:[ 	]+0f 24 0d d3 10[ 	]+fmsubpd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+cbe:[ 	]+0f 24 0d 97 11 00 00 10 00[ 	]+fmsubpd %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+cc7:[ 	]+0f 24 0d 97 19 00 00 10 00[ 	]+fmsubpd %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+cd0:[ 	]+0f 24 12 d3 10[ 	]+fnmaddss %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+cd5:[ 	]+0f 24 12 97 11 00 00 10 00[ 	]+fnmaddss 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+cde:[ 	]+0f 24 12 97 19 00 00 10 00[ 	]+fnmaddss %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+ce7:[ 	]+0f 24 16 d3 10[ 	]+fnmaddss %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+cec:[ 	]+0f 24 16 97 11 00 00 10 00[ 	]+fnmaddss %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+cf5:[ 	]+0f 24 16 97 19 00 00 10 00[ 	]+fnmaddss %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+cfe:[ 	]+0f 24 13 d3 10[ 	]+fnmaddsd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+d03:[ 	]+0f 24 13 97 11 00 00 10 00[ 	]+fnmaddsd 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+d0c:[ 	]+0f 24 13 97 19 00 00 10 00[ 	]+fnmaddsd %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+d15:[ 	]+0f 24 17 d3 10[ 	]+fnmaddsd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+d1a:[ 	]+0f 24 17 97 11 00 00 10 00[ 	]+fnmaddsd %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+d23:[ 	]+0f 24 17 97 19 00 00 10 00[ 	]+fnmaddsd %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+d2c:[ 	]+0f 24 10 d3 10[ 	]+fnmaddps %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+d31:[ 	]+0f 24 10 97 11 00 00 10 00[ 	]+fnmaddps 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+d3a:[ 	]+0f 24 10 97 19 00 00 10 00[ 	]+fnmaddps %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+d43:[ 	]+0f 24 14 d3 10[ 	]+fnmaddps %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+d48:[ 	]+0f 24 14 97 11 00 00 10 00[ 	]+fnmaddps %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+d51:[ 	]+0f 24 14 97 19 00 00 10 00[ 	]+fnmaddps %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+d5a:[ 	]+0f 24 11 d3 10[ 	]+fnmaddpd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+d5f:[ 	]+0f 24 11 97 11 00 00 10 00[ 	]+fnmaddpd 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+d68:[ 	]+0f 24 11 97 19 00 00 10 00[ 	]+fnmaddpd %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+d71:[ 	]+0f 24 15 d3 10[ 	]+fnmaddpd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+d76:[ 	]+0f 24 15 97 11 00 00 10 00[ 	]+fnmaddpd %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+d7f:[ 	]+0f 24 15 97 19 00 00 10 00[ 	]+fnmaddpd %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+d88:[ 	]+0f 24 1a d3 10[ 	]+fnmsubss %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+d8d:[ 	]+0f 24 1a 97 11 00 00 10 00[ 	]+fnmsubss 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+d96:[ 	]+0f 24 1a 97 19 00 00 10 00[ 	]+fnmsubss %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+d9f:[ 	]+0f 24 1e d3 10[ 	]+fnmsubss %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+da4:[ 	]+0f 24 1e 97 11 00 00 10 00[ 	]+fnmsubss %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+dad:[ 	]+0f 24 1e 97 19 00 00 10 00[ 	]+fnmsubss %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+db6:[ 	]+0f 24 1b d3 10[ 	]+fnmsubsd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+dbb:[ 	]+0f 24 1b 97 11 00 00 10 00[ 	]+fnmsubsd 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+dc4:[ 	]+0f 24 1b 97 19 00 00 10 00[ 	]+fnmsubsd %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+dcd:[ 	]+0f 24 1f d3 10[ 	]+fnmsubsd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+dd2:[ 	]+0f 24 1f 97 11 00 00 10 00[ 	]+fnmsubsd %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+ddb:[ 	]+0f 24 1f 97 19 00 00 10 00[ 	]+fnmsubsd %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+de4:[ 	]+0f 24 18 d3 10[ 	]+fnmsubps %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+de9:[ 	]+0f 24 18 97 11 00 00 10 00[ 	]+fnmsubps 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+df2:[ 	]+0f 24 18 97 19 00 00 10 00[ 	]+fnmsubps %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+dfb:[ 	]+0f 24 1c d3 10[ 	]+fnmsubps %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+e00:[ 	]+0f 24 1c 97 11 00 00 10 00[ 	]+fnmsubps %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+e09:[ 	]+0f 24 1c 97 19 00 00 10 00[ 	]+fnmsubps %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+e12:[ 	]+0f 24 19 d3 10[ 	]+fnmsubpd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+e17:[ 	]+0f 24 19 97 11 00 00 10 00[ 	]+fnmsubpd 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+e20:[ 	]+0f 24 19 97 19 00 00 10 00[ 	]+fnmsubpd %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+e29:[ 	]+0f 24 1d d3 10[ 	]+fnmsubpd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+e2e:[ 	]+0f 24 1d 97 11 00 00 10 00[ 	]+fnmsubpd %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+e37:[ 	]+0f 24 1d 97 19 00 00 10 00[ 	]+fnmsubpd %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+e40:[ 	]+0f 24 85 d3 10[ 	]+pmacssww %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+e45:[ 	]+0f 24 85 52 10 04[ 	]+pmacssww %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+e4b:[ 	]+0f 24 95 d3 10[ 	]+pmacsww %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+e50:[ 	]+0f 24 95 52 10 04[ 	]+pmacsww %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+e56:[ 	]+0f 24 96 d3 10[ 	]+pmacswd %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+e5b:[ 	]+0f 24 96 52 10 04[ 	]+pmacswd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+e61:[ 	]+0f 24 8e d3 10[ 	]+pmacssdd %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+e66:[ 	]+0f 24 8e 52 10 04[ 	]+pmacssdd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+e6c:[ 	]+0f 24 9e d3 10[ 	]+pmacsdd %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+e71:[ 	]+0f 24 9e 52 10 04[ 	]+pmacsdd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+e77:[ 	]+0f 24 87 d3 10[ 	]+pmacssdql %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+e7c:[ 	]+0f 24 87 52 10 04[ 	]+pmacssdql %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+e82:[ 	]+0f 24 8f d3 10[ 	]+pmacssdqh %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+e87:[ 	]+0f 24 8f 52 10 04[ 	]+pmacssdqh %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+e8d:[ 	]+0f 24 97 d3 10[ 	]+pmacsdql %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+e92:[ 	]+0f 24 97 52 10 04[ 	]+pmacsdql %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+e98:[ 	]+0f 24 9f d3 10[ 	]+pmacsdqh %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+e9d:[ 	]+0f 24 9f 52 10 04[ 	]+pmacsdqh %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+ea3:[ 	]+0f 24 a6 d3 10[ 	]+pmadcsswd %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+ea8:[ 	]+0f 24 a6 52 10 04[ 	]+pmadcsswd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+eae:[ 	]+0f 24 b6 d3 10[ 	]+pmadcswd %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+eb3:[ 	]+0f 24 b6 52 10 04[ 	]+pmadcswd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+eb9:[ 	]+0f 24 85 e5 b5[ 	]+pmacssww %xmm11,%xmm12,%xmm13,%xmm11
[ 	]+ebe:[ 	]+0f 24 85 a7 b5 00 00 10 00[ 	]+pmacssww %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+ec7:[ 	]+0f 24 95 e5 b5[ 	]+pmacsww %xmm11,%xmm12,%xmm13,%xmm11
[ 	]+ecc:[ 	]+0f 24 95 a7 b5 00 00 10 00[ 	]+pmacsww %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+ed5:[ 	]+0f 24 96 e5 b5[ 	]+pmacswd %xmm11,%xmm12,%xmm13,%xmm11
[ 	]+eda:[ 	]+0f 24 96 a7 b5 00 00 10 00[ 	]+pmacswd %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+ee3:[ 	]+0f 24 8e e5 b5[ 	]+pmacssdd %xmm11,%xmm12,%xmm13,%xmm11
[ 	]+ee8:[ 	]+0f 24 8e a7 b5 00 00 10 00[ 	]+pmacssdd %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+ef1:[ 	]+0f 24 9e e5 b5[ 	]+pmacsdd %xmm11,%xmm12,%xmm13,%xmm11
[ 	]+ef6:[ 	]+0f 24 9e a7 b5 00 00 10 00[ 	]+pmacsdd %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+eff:[ 	]+0f 24 87 e5 b5[ 	]+pmacssdql %xmm11,%xmm12,%xmm13,%xmm11
[ 	]+f04:[ 	]+0f 24 87 a7 b5 00 00 10 00[ 	]+pmacssdql %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+f0d:[ 	]+0f 24 8f e5 b5[ 	]+pmacssdqh %xmm11,%xmm12,%xmm13,%xmm11
[ 	]+f12:[ 	]+0f 24 8f a7 b5 00 00 10 00[ 	]+pmacssdqh %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+f1b:[ 	]+0f 24 97 e5 b5[ 	]+pmacsdql %xmm11,%xmm12,%xmm13,%xmm11
[ 	]+f20:[ 	]+0f 24 97 a7 b5 00 00 10 00[ 	]+pmacsdql %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+f29:[ 	]+0f 24 9f e5 b5[ 	]+pmacsdqh %xmm11,%xmm12,%xmm13,%xmm11
[ 	]+f2e:[ 	]+0f 24 9f a7 b5 00 00 10 00[ 	]+pmacsdqh %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+f37:[ 	]+0f 24 a6 e5 b5[ 	]+pmadcsswd %xmm11,%xmm12,%xmm13,%xmm11
[ 	]+f3c:[ 	]+0f 24 a6 a7 b5 00 00 10 00[ 	]+pmadcsswd %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+f45:[ 	]+0f 24 b6 e5 b5[ 	]+pmadcswd %xmm11,%xmm12,%xmm13,%xmm11
[ 	]+f4a:[ 	]+0f 24 b6 a7 b5 00 00 10 00[ 	]+pmadcswd %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+f53:[ 	]+0f 24 85 e3 14[ 	]+pmacssww %xmm1,%xmm12,%xmm3,%xmm1
[ 	]+f58:[ 	]+0f 24 85 62 14 04[ 	]+pmacssww %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+f5e:[ 	]+0f 24 95 e3 14[ 	]+pmacsww %xmm1,%xmm12,%xmm3,%xmm1
[ 	]+f63:[ 	]+0f 24 95 62 14 04[ 	]+pmacsww %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+f69:[ 	]+0f 24 96 e3 14[ 	]+pmacswd %xmm1,%xmm12,%xmm3,%xmm1
[ 	]+f6e:[ 	]+0f 24 96 62 14 04[ 	]+pmacswd %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+f74:[ 	]+0f 24 8e e3 14[ 	]+pmacssdd %xmm1,%xmm12,%xmm3,%xmm1
[ 	]+f79:[ 	]+0f 24 8e 62 14 04[ 	]+pmacssdd %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+f7f:[ 	]+0f 24 9e e3 14[ 	]+pmacsdd %xmm1,%xmm12,%xmm3,%xmm1
[ 	]+f84:[ 	]+0f 24 9e 62 14 04[ 	]+pmacsdd %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+f8a:[ 	]+0f 24 87 e3 14[ 	]+pmacssdql %xmm1,%xmm12,%xmm3,%xmm1
[ 	]+f8f:[ 	]+0f 24 87 62 14 04[ 	]+pmacssdql %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+f95:[ 	]+0f 24 8f e3 14[ 	]+pmacssdqh %xmm1,%xmm12,%xmm3,%xmm1
[ 	]+f9a:[ 	]+0f 24 8f 62 14 04[ 	]+pmacssdqh %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+fa0:[ 	]+0f 24 97 e3 14[ 	]+pmacsdql %xmm1,%xmm12,%xmm3,%xmm1
[ 	]+fa5:[ 	]+0f 24 97 62 14 04[ 	]+pmacsdql %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+fab:[ 	]+0f 24 9f e3 14[ 	]+pmacsdqh %xmm1,%xmm12,%xmm3,%xmm1
[ 	]+fb0:[ 	]+0f 24 9f 62 14 04[ 	]+pmacsdqh %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+fb6:[ 	]+0f 24 a6 e3 14[ 	]+pmadcsswd %xmm1,%xmm12,%xmm3,%xmm1
[ 	]+fbb:[ 	]+0f 24 a6 62 14 04[ 	]+pmadcsswd %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+fc1:[ 	]+0f 24 b6 e3 14[ 	]+pmadcswd %xmm1,%xmm12,%xmm3,%xmm1
[ 	]+fc6:[ 	]+0f 24 b6 62 14 04[ 	]+pmadcswd %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+fcc:[ 	]+0f 24 85 d3 b0[ 	]+pmacssww %xmm11,%xmm2,%xmm3,%xmm11
[ 	]+fd1:[ 	]+0f 24 85 52 b0 04[ 	]+pmacssww %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+fd7:[ 	]+0f 24 95 d3 b0[ 	]+pmacsww %xmm11,%xmm2,%xmm3,%xmm11
[ 	]+fdc:[ 	]+0f 24 95 52 b0 04[ 	]+pmacsww %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+fe2:[ 	]+0f 24 96 d3 b0[ 	]+pmacswd %xmm11,%xmm2,%xmm3,%xmm11
[ 	]+fe7:[ 	]+0f 24 96 52 b0 04[ 	]+pmacswd %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+fed:[ 	]+0f 24 8e d3 b0[ 	]+pmacssdd %xmm11,%xmm2,%xmm3,%xmm11
[ 	]+ff2:[ 	]+0f 24 8e 52 b0 04[ 	]+pmacssdd %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+ff8:[ 	]+0f 24 9e d3 b0[ 	]+pmacsdd %xmm11,%xmm2,%xmm3,%xmm11
[ 	]+ffd:[ 	]+0f 24 9e 52 b0 04[ 	]+pmacsdd %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1003:[ 	]+0f 24 87 d3 b0[ 	]+pmacssdql %xmm11,%xmm2,%xmm3,%xmm11
[ 	]+1008:[ 	]+0f 24 87 52 b0 04[ 	]+pmacssdql %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+100e:[ 	]+0f 24 8f d3 b0[ 	]+pmacssdqh %xmm11,%xmm2,%xmm3,%xmm11
[ 	]+1013:[ 	]+0f 24 8f 52 b0 04[ 	]+pmacssdqh %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1019:[ 	]+0f 24 97 d3 b0[ 	]+pmacsdql %xmm11,%xmm2,%xmm3,%xmm11
[ 	]+101e:[ 	]+0f 24 97 52 b0 04[ 	]+pmacsdql %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1024:[ 	]+0f 24 9f d3 b0[ 	]+pmacsdqh %xmm11,%xmm2,%xmm3,%xmm11
[ 	]+1029:[ 	]+0f 24 9f 52 b0 04[ 	]+pmacsdqh %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+102f:[ 	]+0f 24 a6 d3 b0[ 	]+pmadcsswd %xmm11,%xmm2,%xmm3,%xmm11
[ 	]+1034:[ 	]+0f 24 a6 52 b0 04[ 	]+pmadcsswd %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+103a:[ 	]+0f 24 b6 d3 b0[ 	]+pmadcswd %xmm11,%xmm2,%xmm3,%xmm11
[ 	]+103f:[ 	]+0f 24 b6 52 b0 04[ 	]+pmadcswd %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1045:[ 	]+0f 24 85 d5 11[ 	]+pmacssww %xmm1,%xmm2,%xmm13,%xmm1
[ 	]+104a:[ 	]+0f 24 85 52 10 04[ 	]+pmacssww %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1050:[ 	]+0f 24 95 d5 11[ 	]+pmacsww %xmm1,%xmm2,%xmm13,%xmm1
[ 	]+1055:[ 	]+0f 24 95 52 10 04[ 	]+pmacsww %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+105b:[ 	]+0f 24 96 d5 11[ 	]+pmacswd %xmm1,%xmm2,%xmm13,%xmm1
[ 	]+1060:[ 	]+0f 24 96 52 10 04[ 	]+pmacswd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1066:[ 	]+0f 24 8e d5 11[ 	]+pmacssdd %xmm1,%xmm2,%xmm13,%xmm1
[ 	]+106b:[ 	]+0f 24 8e 52 10 04[ 	]+pmacssdd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1071:[ 	]+0f 24 9e d5 11[ 	]+pmacsdd %xmm1,%xmm2,%xmm13,%xmm1
[ 	]+1076:[ 	]+0f 24 9e 52 10 04[ 	]+pmacsdd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+107c:[ 	]+0f 24 87 d5 11[ 	]+pmacssdql %xmm1,%xmm2,%xmm13,%xmm1
[ 	]+1081:[ 	]+0f 24 87 52 10 04[ 	]+pmacssdql %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1087:[ 	]+0f 24 8f d5 11[ 	]+pmacssdqh %xmm1,%xmm2,%xmm13,%xmm1
[ 	]+108c:[ 	]+0f 24 8f 52 10 04[ 	]+pmacssdqh %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1092:[ 	]+0f 24 97 d5 11[ 	]+pmacsdql %xmm1,%xmm2,%xmm13,%xmm1
[ 	]+1097:[ 	]+0f 24 97 52 10 04[ 	]+pmacsdql %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+109d:[ 	]+0f 24 9f d5 11[ 	]+pmacsdqh %xmm1,%xmm2,%xmm13,%xmm1
[ 	]+10a2:[ 	]+0f 24 9f 52 10 04[ 	]+pmacsdqh %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+10a8:[ 	]+0f 24 a6 d5 11[ 	]+pmadcsswd %xmm1,%xmm2,%xmm13,%xmm1
[ 	]+10ad:[ 	]+0f 24 a6 52 10 04[ 	]+pmadcsswd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+10b3:[ 	]+0f 24 b6 d5 11[ 	]+pmadcswd %xmm1,%xmm2,%xmm13,%xmm1
[ 	]+10b8:[ 	]+0f 24 b6 52 10 04[ 	]+pmadcswd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+10be:[ 	]+0f 24 85 d3 10[ 	]+pmacssww %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+10c3:[ 	]+0f 24 85 97 11 00 00 10 00[ 	]+pmacssww %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+10cc:[ 	]+0f 24 95 d3 10[ 	]+pmacsww %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+10d1:[ 	]+0f 24 95 97 11 00 00 10 00[ 	]+pmacsww %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+10da:[ 	]+0f 24 96 d3 10[ 	]+pmacswd %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+10df:[ 	]+0f 24 96 97 11 00 00 10 00[ 	]+pmacswd %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+10e8:[ 	]+0f 24 8e d3 10[ 	]+pmacssdd %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+10ed:[ 	]+0f 24 8e 97 11 00 00 10 00[ 	]+pmacssdd %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+10f6:[ 	]+0f 24 9e d3 10[ 	]+pmacsdd %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+10fb:[ 	]+0f 24 9e 97 11 00 00 10 00[ 	]+pmacsdd %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+1104:[ 	]+0f 24 87 d3 10[ 	]+pmacssdql %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+1109:[ 	]+0f 24 87 97 11 00 00 10 00[ 	]+pmacssdql %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+1112:[ 	]+0f 24 8f d3 10[ 	]+pmacssdqh %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+1117:[ 	]+0f 24 8f 97 11 00 00 10 00[ 	]+pmacssdqh %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+1120:[ 	]+0f 24 97 d3 10[ 	]+pmacsdql %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+1125:[ 	]+0f 24 97 97 11 00 00 10 00[ 	]+pmacsdql %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+112e:[ 	]+0f 24 9f d3 10[ 	]+pmacsdqh %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+1133:[ 	]+0f 24 9f 97 11 00 00 10 00[ 	]+pmacsdqh %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+113c:[ 	]+0f 24 a6 d3 10[ 	]+pmadcsswd %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+1141:[ 	]+0f 24 a6 97 11 00 00 10 00[ 	]+pmadcsswd %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+114a:[ 	]+0f 24 b6 d3 10[ 	]+pmadcswd %xmm1,%xmm2,%xmm3,%xmm1
[ 	]+114f:[ 	]+0f 24 b6 97 11 00 00 10 00[ 	]+pmadcswd %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+1158:[ 	]+0f 7a 41 ca[ 	]+phaddbw %xmm2,%xmm1
[ 	]+115c:[ 	]+0f 7a 41 4a 04[ 	]+phaddbw 0x4\(%rdx\),%xmm1
[ 	]+1161:[ 	]+0f 7a 42 ca[ 	]+phaddbd %xmm2,%xmm1
[ 	]+1165:[ 	]+0f 7a 42 4a 04[ 	]+phaddbd 0x4\(%rdx\),%xmm1
[ 	]+116a:[ 	]+0f 7a 43 ca[ 	]+phaddbq %xmm2,%xmm1
[ 	]+116e:[ 	]+0f 7a 43 4a 04[ 	]+phaddbq 0x4\(%rdx\),%xmm1
[ 	]+1173:[ 	]+0f 7a 46 ca[ 	]+phaddwd %xmm2,%xmm1
[ 	]+1177:[ 	]+0f 7a 46 4a 04[ 	]+phaddwd 0x4\(%rdx\),%xmm1
[ 	]+117c:[ 	]+0f 7a 47 ca[ 	]+phaddwq %xmm2,%xmm1
[ 	]+1180:[ 	]+0f 7a 47 4a 04[ 	]+phaddwq 0x4\(%rdx\),%xmm1
[ 	]+1185:[ 	]+0f 7a 4b ca[ 	]+phadddq %xmm2,%xmm1
[ 	]+1189:[ 	]+0f 7a 4b 4a 04[ 	]+phadddq 0x4\(%rdx\),%xmm1
[ 	]+118e:[ 	]+0f 7a 51 ca[ 	]+phaddubw %xmm2,%xmm1
[ 	]+1192:[ 	]+0f 7a 51 4a 04[ 	]+phaddubw 0x4\(%rdx\),%xmm1
[ 	]+1197:[ 	]+0f 7a 52 ca[ 	]+phaddubd %xmm2,%xmm1
[ 	]+119b:[ 	]+0f 7a 52 4a 04[ 	]+phaddubd 0x4\(%rdx\),%xmm1
[ 	]+11a0:[ 	]+0f 7a 53 ca[ 	]+phaddubq %xmm2,%xmm1
[ 	]+11a4:[ 	]+0f 7a 53 4a 04[ 	]+phaddubq 0x4\(%rdx\),%xmm1
[ 	]+11a9:[ 	]+0f 7a 56 ca[ 	]+phadduwd %xmm2,%xmm1
[ 	]+11ad:[ 	]+0f 7a 56 4a 04[ 	]+phadduwd 0x4\(%rdx\),%xmm1
[ 	]+11b2:[ 	]+0f 7a 57 ca[ 	]+phadduwq %xmm2,%xmm1
[ 	]+11b6:[ 	]+0f 7a 57 4a 04[ 	]+phadduwq 0x4\(%rdx\),%xmm1
[ 	]+11bb:[ 	]+0f 7a 5b ca[ 	]+phaddudq %xmm2,%xmm1
[ 	]+11bf:[ 	]+0f 7a 5b 4a 04[ 	]+phaddudq 0x4\(%rdx\),%xmm1
[ 	]+11c4:[ 	]+0f 7a 61 ca[ 	]+phsubbw %xmm2,%xmm1
[ 	]+11c8:[ 	]+0f 7a 61 4a 04[ 	]+phsubbw 0x4\(%rdx\),%xmm1
[ 	]+11cd:[ 	]+0f 7a 62 ca[ 	]+phsubbd %xmm2,%xmm1
[ 	]+11d1:[ 	]+0f 7a 62 4a 04[ 	]+phsubbd 0x4\(%rdx\),%xmm1
[ 	]+11d6:[ 	]+0f 7a 63 ca[ 	]+phsubbq %xmm2,%xmm1
[ 	]+11da:[ 	]+0f 7a 63 4a 04[ 	]+phsubbq 0x4\(%rdx\),%xmm1
[ 	]+11df:[ 	]+45 0f 7a 41 dc[ 	]+phaddbw %xmm12,%xmm11
[ 	]+11e4:[ 	]+45 0f 7a 41 9f 00 00 10 00[ 	]+phaddbw 0x100000\(%r15\),%xmm11
[ 	]+11ed:[ 	]+45 0f 7a 42 dc[ 	]+phaddbd %xmm12,%xmm11
[ 	]+11f2:[ 	]+45 0f 7a 42 9f 00 00 10 00[ 	]+phaddbd 0x100000\(%r15\),%xmm11
[ 	]+11fb:[ 	]+45 0f 7a 43 dc[ 	]+phaddbq %xmm12,%xmm11
[ 	]+1200:[ 	]+45 0f 7a 43 9f 00 00 10 00[ 	]+phaddbq 0x100000\(%r15\),%xmm11
[ 	]+1209:[ 	]+45 0f 7a 46 dc[ 	]+phaddwd %xmm12,%xmm11
[ 	]+120e:[ 	]+45 0f 7a 46 9f 00 00 10 00[ 	]+phaddwd 0x100000\(%r15\),%xmm11
[ 	]+1217:[ 	]+45 0f 7a 47 dc[ 	]+phaddwq %xmm12,%xmm11
[ 	]+121c:[ 	]+45 0f 7a 47 9f 00 00 10 00[ 	]+phaddwq 0x100000\(%r15\),%xmm11
[ 	]+1225:[ 	]+45 0f 7a 4b dc[ 	]+phadddq %xmm12,%xmm11
[ 	]+122a:[ 	]+45 0f 7a 4b 9f 00 00 10 00[ 	]+phadddq 0x100000\(%r15\),%xmm11
[ 	]+1233:[ 	]+45 0f 7a 51 dc[ 	]+phaddubw %xmm12,%xmm11
[ 	]+1238:[ 	]+45 0f 7a 51 9f 00 00 10 00[ 	]+phaddubw 0x100000\(%r15\),%xmm11
[ 	]+1241:[ 	]+45 0f 7a 52 dc[ 	]+phaddubd %xmm12,%xmm11
[ 	]+1246:[ 	]+45 0f 7a 52 9f 00 00 10 00[ 	]+phaddubd 0x100000\(%r15\),%xmm11
[ 	]+124f:[ 	]+45 0f 7a 53 dc[ 	]+phaddubq %xmm12,%xmm11
[ 	]+1254:[ 	]+45 0f 7a 53 9f 00 00 10 00[ 	]+phaddubq 0x100000\(%r15\),%xmm11
[ 	]+125d:[ 	]+45 0f 7a 56 dc[ 	]+phadduwd %xmm12,%xmm11
[ 	]+1262:[ 	]+45 0f 7a 56 9f 00 00 10 00[ 	]+phadduwd 0x100000\(%r15\),%xmm11
[ 	]+126b:[ 	]+45 0f 7a 57 dc[ 	]+phadduwq %xmm12,%xmm11
[ 	]+1270:[ 	]+45 0f 7a 57 9f 00 00 10 00[ 	]+phadduwq 0x100000\(%r15\),%xmm11
[ 	]+1279:[ 	]+45 0f 7a 5b dc[ 	]+phaddudq %xmm12,%xmm11
[ 	]+127e:[ 	]+45 0f 7a 5b 9f 00 00 10 00[ 	]+phaddudq 0x100000\(%r15\),%xmm11
[ 	]+1287:[ 	]+45 0f 7a 61 dc[ 	]+phsubbw %xmm12,%xmm11
[ 	]+128c:[ 	]+45 0f 7a 61 9f 00 00 10 00[ 	]+phsubbw 0x100000\(%r15\),%xmm11
[ 	]+1295:[ 	]+45 0f 7a 62 dc[ 	]+phsubbd %xmm12,%xmm11
[ 	]+129a:[ 	]+45 0f 7a 62 9f 00 00 10 00[ 	]+phsubbd 0x100000\(%r15\),%xmm11
[ 	]+12a3:[ 	]+45 0f 7a 63 dc[ 	]+phsubbq %xmm12,%xmm11
[ 	]+12a8:[ 	]+45 0f 7a 63 9f 00 00 10 00[ 	]+phsubbq 0x100000\(%r15\),%xmm11
[ 	]+12b1:[ 	]+41 0f 7a 41 cc[ 	]+phaddbw %xmm12,%xmm1
[ 	]+12b6:[ 	]+0f 7a 41 4a 04[ 	]+phaddbw 0x4\(%rdx\),%xmm1
[ 	]+12bb:[ 	]+41 0f 7a 42 cc[ 	]+phaddbd %xmm12,%xmm1
[ 	]+12c0:[ 	]+0f 7a 42 4a 04[ 	]+phaddbd 0x4\(%rdx\),%xmm1
[ 	]+12c5:[ 	]+41 0f 7a 43 cc[ 	]+phaddbq %xmm12,%xmm1
[ 	]+12ca:[ 	]+0f 7a 43 4a 04[ 	]+phaddbq 0x4\(%rdx\),%xmm1
[ 	]+12cf:[ 	]+41 0f 7a 46 cc[ 	]+phaddwd %xmm12,%xmm1
[ 	]+12d4:[ 	]+0f 7a 46 4a 04[ 	]+phaddwd 0x4\(%rdx\),%xmm1
[ 	]+12d9:[ 	]+41 0f 7a 47 cc[ 	]+phaddwq %xmm12,%xmm1
[ 	]+12de:[ 	]+0f 7a 47 4a 04[ 	]+phaddwq 0x4\(%rdx\),%xmm1
[ 	]+12e3:[ 	]+41 0f 7a 4b cc[ 	]+phadddq %xmm12,%xmm1
[ 	]+12e8:[ 	]+0f 7a 4b 4a 04[ 	]+phadddq 0x4\(%rdx\),%xmm1
[ 	]+12ed:[ 	]+41 0f 7a 51 cc[ 	]+phaddubw %xmm12,%xmm1
[ 	]+12f2:[ 	]+0f 7a 51 4a 04[ 	]+phaddubw 0x4\(%rdx\),%xmm1
[ 	]+12f7:[ 	]+41 0f 7a 52 cc[ 	]+phaddubd %xmm12,%xmm1
[ 	]+12fc:[ 	]+0f 7a 52 4a 04[ 	]+phaddubd 0x4\(%rdx\),%xmm1
[ 	]+1301:[ 	]+41 0f 7a 53 cc[ 	]+phaddubq %xmm12,%xmm1
[ 	]+1306:[ 	]+0f 7a 53 4a 04[ 	]+phaddubq 0x4\(%rdx\),%xmm1
[ 	]+130b:[ 	]+41 0f 7a 56 cc[ 	]+phadduwd %xmm12,%xmm1
[ 	]+1310:[ 	]+0f 7a 56 4a 04[ 	]+phadduwd 0x4\(%rdx\),%xmm1
[ 	]+1315:[ 	]+41 0f 7a 57 cc[ 	]+phadduwq %xmm12,%xmm1
[ 	]+131a:[ 	]+0f 7a 57 4a 04[ 	]+phadduwq 0x4\(%rdx\),%xmm1
[ 	]+131f:[ 	]+41 0f 7a 5b cc[ 	]+phaddudq %xmm12,%xmm1
[ 	]+1324:[ 	]+0f 7a 5b 4a 04[ 	]+phaddudq 0x4\(%rdx\),%xmm1
[ 	]+1329:[ 	]+41 0f 7a 61 cc[ 	]+phsubbw %xmm12,%xmm1
[ 	]+132e:[ 	]+0f 7a 61 4a 04[ 	]+phsubbw 0x4\(%rdx\),%xmm1
[ 	]+1333:[ 	]+41 0f 7a 62 cc[ 	]+phsubbd %xmm12,%xmm1
[ 	]+1338:[ 	]+0f 7a 62 4a 04[ 	]+phsubbd 0x4\(%rdx\),%xmm1
[ 	]+133d:[ 	]+41 0f 7a 63 cc[ 	]+phsubbq %xmm12,%xmm1
[ 	]+1342:[ 	]+0f 7a 63 4a 04[ 	]+phsubbq 0x4\(%rdx\),%xmm1
[ 	]+1347:[ 	]+44 0f 7a 41 da[ 	]+phaddbw %xmm2,%xmm11
[ 	]+134c:[ 	]+44 0f 7a 41 5a 04[ 	]+phaddbw 0x4\(%rdx\),%xmm11
[ 	]+1352:[ 	]+44 0f 7a 42 da[ 	]+phaddbd %xmm2,%xmm11
[ 	]+1357:[ 	]+44 0f 7a 42 5a 04[ 	]+phaddbd 0x4\(%rdx\),%xmm11
[ 	]+135d:[ 	]+44 0f 7a 43 da[ 	]+phaddbq %xmm2,%xmm11
[ 	]+1362:[ 	]+44 0f 7a 43 5a 04[ 	]+phaddbq 0x4\(%rdx\),%xmm11
[ 	]+1368:[ 	]+44 0f 7a 46 da[ 	]+phaddwd %xmm2,%xmm11
[ 	]+136d:[ 	]+44 0f 7a 46 5a 04[ 	]+phaddwd 0x4\(%rdx\),%xmm11
[ 	]+1373:[ 	]+44 0f 7a 47 da[ 	]+phaddwq %xmm2,%xmm11
[ 	]+1378:[ 	]+44 0f 7a 47 5a 04[ 	]+phaddwq 0x4\(%rdx\),%xmm11
[ 	]+137e:[ 	]+44 0f 7a 4b da[ 	]+phadddq %xmm2,%xmm11
[ 	]+1383:[ 	]+44 0f 7a 4b 5a 04[ 	]+phadddq 0x4\(%rdx\),%xmm11
[ 	]+1389:[ 	]+44 0f 7a 51 da[ 	]+phaddubw %xmm2,%xmm11
[ 	]+138e:[ 	]+44 0f 7a 51 5a 04[ 	]+phaddubw 0x4\(%rdx\),%xmm11
[ 	]+1394:[ 	]+44 0f 7a 52 da[ 	]+phaddubd %xmm2,%xmm11
[ 	]+1399:[ 	]+44 0f 7a 52 5a 04[ 	]+phaddubd 0x4\(%rdx\),%xmm11
[ 	]+139f:[ 	]+44 0f 7a 53 da[ 	]+phaddubq %xmm2,%xmm11
[ 	]+13a4:[ 	]+44 0f 7a 53 5a 04[ 	]+phaddubq 0x4\(%rdx\),%xmm11
[ 	]+13aa:[ 	]+44 0f 7a 56 da[ 	]+phadduwd %xmm2,%xmm11
[ 	]+13af:[ 	]+44 0f 7a 56 5a 04[ 	]+phadduwd 0x4\(%rdx\),%xmm11
[ 	]+13b5:[ 	]+44 0f 7a 57 da[ 	]+phadduwq %xmm2,%xmm11
[ 	]+13ba:[ 	]+44 0f 7a 57 5a 04[ 	]+phadduwq 0x4\(%rdx\),%xmm11
[ 	]+13c0:[ 	]+44 0f 7a 5b da[ 	]+phaddudq %xmm2,%xmm11
[ 	]+13c5:[ 	]+44 0f 7a 5b 5a 04[ 	]+phaddudq 0x4\(%rdx\),%xmm11
[ 	]+13cb:[ 	]+44 0f 7a 61 da[ 	]+phsubbw %xmm2,%xmm11
[ 	]+13d0:[ 	]+44 0f 7a 61 5a 04[ 	]+phsubbw 0x4\(%rdx\),%xmm11
[ 	]+13d6:[ 	]+44 0f 7a 62 da[ 	]+phsubbd %xmm2,%xmm11
[ 	]+13db:[ 	]+44 0f 7a 62 5a 04[ 	]+phsubbd 0x4\(%rdx\),%xmm11
[ 	]+13e1:[ 	]+44 0f 7a 63 da[ 	]+phsubbq %xmm2,%xmm11
[ 	]+13e6:[ 	]+44 0f 7a 63 5a 04[ 	]+phsubbq 0x4\(%rdx\),%xmm11
[ 	]+13ec:[ 	]+0f 7a 41 ca[ 	]+phaddbw %xmm2,%xmm1
[ 	]+13f0:[ 	]+0f 7a 41 4a 04[ 	]+phaddbw 0x4\(%rdx\),%xmm1
[ 	]+13f5:[ 	]+0f 7a 42 ca[ 	]+phaddbd %xmm2,%xmm1
[ 	]+13f9:[ 	]+0f 7a 42 4a 04[ 	]+phaddbd 0x4\(%rdx\),%xmm1
[ 	]+13fe:[ 	]+0f 7a 43 ca[ 	]+phaddbq %xmm2,%xmm1
[ 	]+1402:[ 	]+0f 7a 43 4a 04[ 	]+phaddbq 0x4\(%rdx\),%xmm1
[ 	]+1407:[ 	]+0f 7a 46 ca[ 	]+phaddwd %xmm2,%xmm1
[ 	]+140b:[ 	]+0f 7a 46 4a 04[ 	]+phaddwd 0x4\(%rdx\),%xmm1
[ 	]+1410:[ 	]+0f 7a 47 ca[ 	]+phaddwq %xmm2,%xmm1
[ 	]+1414:[ 	]+0f 7a 47 4a 04[ 	]+phaddwq 0x4\(%rdx\),%xmm1
[ 	]+1419:[ 	]+0f 7a 4b ca[ 	]+phadddq %xmm2,%xmm1
[ 	]+141d:[ 	]+0f 7a 4b 4a 04[ 	]+phadddq 0x4\(%rdx\),%xmm1
[ 	]+1422:[ 	]+0f 7a 51 ca[ 	]+phaddubw %xmm2,%xmm1
[ 	]+1426:[ 	]+0f 7a 51 4a 04[ 	]+phaddubw 0x4\(%rdx\),%xmm1
[ 	]+142b:[ 	]+0f 7a 52 ca[ 	]+phaddubd %xmm2,%xmm1
[ 	]+142f:[ 	]+0f 7a 52 4a 04[ 	]+phaddubd 0x4\(%rdx\),%xmm1
[ 	]+1434:[ 	]+0f 7a 53 ca[ 	]+phaddubq %xmm2,%xmm1
[ 	]+1438:[ 	]+0f 7a 53 4a 04[ 	]+phaddubq 0x4\(%rdx\),%xmm1
[ 	]+143d:[ 	]+0f 7a 56 ca[ 	]+phadduwd %xmm2,%xmm1
[ 	]+1441:[ 	]+0f 7a 56 4a 04[ 	]+phadduwd 0x4\(%rdx\),%xmm1
[ 	]+1446:[ 	]+0f 7a 57 ca[ 	]+phadduwq %xmm2,%xmm1
[ 	]+144a:[ 	]+0f 7a 57 4a 04[ 	]+phadduwq 0x4\(%rdx\),%xmm1
[ 	]+144f:[ 	]+0f 7a 5b ca[ 	]+phaddudq %xmm2,%xmm1
[ 	]+1453:[ 	]+0f 7a 5b 4a 04[ 	]+phaddudq 0x4\(%rdx\),%xmm1
[ 	]+1458:[ 	]+0f 7a 61 ca[ 	]+phsubbw %xmm2,%xmm1
[ 	]+145c:[ 	]+0f 7a 61 4a 04[ 	]+phsubbw 0x4\(%rdx\),%xmm1
[ 	]+1461:[ 	]+0f 7a 62 ca[ 	]+phsubbd %xmm2,%xmm1
[ 	]+1465:[ 	]+0f 7a 62 4a 04[ 	]+phsubbd 0x4\(%rdx\),%xmm1
[ 	]+146a:[ 	]+0f 7a 63 ca[ 	]+phsubbq %xmm2,%xmm1
[ 	]+146e:[ 	]+0f 7a 63 4a 04[ 	]+phsubbq 0x4\(%rdx\),%xmm1
[ 	]+1473:[ 	]+0f 7a 41 ca[ 	]+phaddbw %xmm2,%xmm1
[ 	]+1477:[ 	]+41 0f 7a 41 8f 00 00 10 00[ 	]+phaddbw 0x100000\(%r15\),%xmm1
[ 	]+1480:[ 	]+0f 7a 42 ca[ 	]+phaddbd %xmm2,%xmm1
[ 	]+1484:[ 	]+41 0f 7a 42 8f 00 00 10 00[ 	]+phaddbd 0x100000\(%r15\),%xmm1
[ 	]+148d:[ 	]+0f 7a 43 ca[ 	]+phaddbq %xmm2,%xmm1
[ 	]+1491:[ 	]+41 0f 7a 43 8f 00 00 10 00[ 	]+phaddbq 0x100000\(%r15\),%xmm1
[ 	]+149a:[ 	]+0f 7a 46 ca[ 	]+phaddwd %xmm2,%xmm1
[ 	]+149e:[ 	]+41 0f 7a 46 8f 00 00 10 00[ 	]+phaddwd 0x100000\(%r15\),%xmm1
[ 	]+14a7:[ 	]+0f 7a 47 ca[ 	]+phaddwq %xmm2,%xmm1
[ 	]+14ab:[ 	]+41 0f 7a 47 8f 00 00 10 00[ 	]+phaddwq 0x100000\(%r15\),%xmm1
[ 	]+14b4:[ 	]+0f 7a 4b ca[ 	]+phadddq %xmm2,%xmm1
[ 	]+14b8:[ 	]+41 0f 7a 4b 8f 00 00 10 00[ 	]+phadddq 0x100000\(%r15\),%xmm1
[ 	]+14c1:[ 	]+0f 7a 51 ca[ 	]+phaddubw %xmm2,%xmm1
[ 	]+14c5:[ 	]+41 0f 7a 51 8f 00 00 10 00[ 	]+phaddubw 0x100000\(%r15\),%xmm1
[ 	]+14ce:[ 	]+0f 7a 52 ca[ 	]+phaddubd %xmm2,%xmm1
[ 	]+14d2:[ 	]+41 0f 7a 52 8f 00 00 10 00[ 	]+phaddubd 0x100000\(%r15\),%xmm1
[ 	]+14db:[ 	]+0f 7a 53 ca[ 	]+phaddubq %xmm2,%xmm1
[ 	]+14df:[ 	]+41 0f 7a 53 8f 00 00 10 00[ 	]+phaddubq 0x100000\(%r15\),%xmm1
[ 	]+14e8:[ 	]+0f 7a 56 ca[ 	]+phadduwd %xmm2,%xmm1
[ 	]+14ec:[ 	]+41 0f 7a 56 8f 00 00 10 00[ 	]+phadduwd 0x100000\(%r15\),%xmm1
[ 	]+14f5:[ 	]+0f 7a 57 ca[ 	]+phadduwq %xmm2,%xmm1
[ 	]+14f9:[ 	]+41 0f 7a 57 8f 00 00 10 00[ 	]+phadduwq 0x100000\(%r15\),%xmm1
[ 	]+1502:[ 	]+0f 7a 5b ca[ 	]+phaddudq %xmm2,%xmm1
[ 	]+1506:[ 	]+41 0f 7a 5b 8f 00 00 10 00[ 	]+phaddudq 0x100000\(%r15\),%xmm1
[ 	]+150f:[ 	]+0f 7a 61 ca[ 	]+phsubbw %xmm2,%xmm1
[ 	]+1513:[ 	]+41 0f 7a 61 8f 00 00 10 00[ 	]+phsubbw 0x100000\(%r15\),%xmm1
[ 	]+151c:[ 	]+0f 7a 62 ca[ 	]+phsubbd %xmm2,%xmm1
[ 	]+1520:[ 	]+41 0f 7a 62 8f 00 00 10 00[ 	]+phsubbd 0x100000\(%r15\),%xmm1
[ 	]+1529:[ 	]+0f 7a 63 ca[ 	]+phsubbq %xmm2,%xmm1
[ 	]+152d:[ 	]+41 0f 7a 63 8f 00 00 10 00[ 	]+phsubbq 0x100000\(%r15\),%xmm1
[ 	]+1536:[ 	]+0f 24 22 d3 10[ 	]+pcmov[ 	]+%xmm3,%xmm2,%xmm1,%xmm1
[ 	]+153b:[ 	]+0f 24 22 52 10 04[ 	]+pcmov[ 	]+0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+1541:[ 	]+0f 24 22 52 18 04[ 	]+pcmov[ 	]+%xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+1547:[ 	]+0f 24 26 d3 10[ 	]+pcmov[ 	]+%xmm1,%xmm3,%xmm2,%xmm1
[ 	]+154c:[ 	]+0f 24 26 52 10 04[ 	]+pcmov[ 	]+%xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1552:[ 	]+0f 24 26 52 18 04[ 	]+pcmov[ 	]+%xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1558:[ 	]+0f 24 23 d3 10[ 	]+pperm[ 	]+%xmm3,%xmm2,%xmm1,%xmm1
[ 	]+155d:[ 	]+0f 24 23 52 10 04[ 	]+pperm[ 	]+0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+1563:[ 	]+0f 24 23 52 18 04[ 	]+pperm[ 	]+%xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+1569:[ 	]+0f 24 27 d3 10[ 	]+pperm[ 	]+%xmm1,%xmm3,%xmm2,%xmm1
[ 	]+156e:[ 	]+0f 24 27 52 10 04[ 	]+pperm[ 	]+%xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1574:[ 	]+0f 24 27 52 18 04[ 	]+pperm[ 	]+%xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+157a:[ 	]+0f 24 20 d3 10[ 	]+permps %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+157f:[ 	]+0f 24 20 52 10 04[ 	]+permps 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+1585:[ 	]+0f 24 20 52 18 04[ 	]+permps %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+158b:[ 	]+0f 24 24 d3 10[ 	]+permps %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+1590:[ 	]+0f 24 24 52 10 04[ 	]+permps %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1596:[ 	]+0f 24 24 52 18 04[ 	]+permps %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+159c:[ 	]+0f 24 21 d3 10[ 	]+permpd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+15a1:[ 	]+0f 24 21 52 10 04[ 	]+permpd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+15a7:[ 	]+0f 24 21 52 18 04[ 	]+permpd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+15ad:[ 	]+0f 24 25 d3 10[ 	]+permpd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+15b2:[ 	]+0f 24 25 52 10 04[ 	]+permpd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+15b8:[ 	]+0f 24 25 52 18 04[ 	]+permpd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+15be:[ 	]+0f 24 22 e5 b5[ 	]+pcmov[ 	]+%xmm13,%xmm12,%xmm11,%xmm11
[ 	]+15c3:[ 	]+0f 24 22 a7 b5 00 00 10 00[ 	]+pcmov[ 	]+0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+15cc:[ 	]+0f 24 22 a7 bd 00 00 10 00[ 	]+pcmov[ 	]+%xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+15d5:[ 	]+0f 24 26 e5 b5[ 	]+pcmov[ 	]+%xmm11,%xmm13,%xmm12,%xmm11
[ 	]+15da:[ 	]+0f 24 26 a7 b5 00 00 10 00[ 	]+pcmov[ 	]+%xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+15e3:[ 	]+0f 24 26 a7 bd 00 00 10 00[ 	]+pcmov[ 	]+%xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+15ec:[ 	]+0f 24 23 e5 b5[ 	]+pperm[ 	]+%xmm13,%xmm12,%xmm11,%xmm11
[ 	]+15f1:[ 	]+0f 24 23 a7 b5 00 00 10 00[ 	]+pperm[ 	]+0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+15fa:[ 	]+0f 24 23 a7 bd 00 00 10 00[ 	]+pperm[ 	]+%xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+1603:[ 	]+0f 24 27 e5 b5[ 	]+pperm[ 	]+%xmm11,%xmm13,%xmm12,%xmm11
[ 	]+1608:[ 	]+0f 24 27 a7 b5 00 00 10 00[ 	]+pperm[ 	]+%xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+1611:[ 	]+0f 24 27 a7 bd 00 00 10 00[ 	]+pperm[ 	]+%xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+161a:[ 	]+0f 24 20 e5 b5[ 	]+permps %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+161f:[ 	]+0f 24 20 a7 b5 00 00 10 00[ 	]+permps 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+1628:[ 	]+0f 24 20 a7 bd 00 00 10 00[ 	]+permps %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+1631:[ 	]+0f 24 24 e5 b5[ 	]+permps %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+1636:[ 	]+0f 24 24 a7 b5 00 00 10 00[ 	]+permps %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+163f:[ 	]+0f 24 24 a7 bd 00 00 10 00[ 	]+permps %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+1648:[ 	]+0f 24 21 e5 b5[ 	]+permpd %xmm13,%xmm12,%xmm11,%xmm11
[ 	]+164d:[ 	]+0f 24 21 a7 b5 00 00 10 00[ 	]+permpd 0x100000\(%r15\),%xmm12,%xmm11,%xmm11
[ 	]+1656:[ 	]+0f 24 21 a7 bd 00 00 10 00[ 	]+permpd %xmm12,0x100000\(%r15\),%xmm11,%xmm11
[ 	]+165f:[ 	]+0f 24 25 e5 b5[ 	]+permpd %xmm11,%xmm13,%xmm12,%xmm11
[ 	]+1664:[ 	]+0f 24 25 a7 b5 00 00 10 00[ 	]+permpd %xmm11,0x100000\(%r15\),%xmm12,%xmm11
[ 	]+166d:[ 	]+0f 24 25 a7 bd 00 00 10 00[ 	]+permpd %xmm11,%xmm12,0x100000\(%r15\),%xmm11
[ 	]+1676:[ 	]+0f 24 22 e3 14[ 	]+pcmov[ 	]+%xmm3,%xmm12,%xmm1,%xmm1
[ 	]+167b:[ 	]+0f 24 22 62 14 04[ 	]+pcmov[ 	]+0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+1681:[ 	]+0f 24 22 62 1c 04[ 	]+pcmov[ 	]+%xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+1687:[ 	]+0f 24 26 e3 14[ 	]+pcmov[ 	]+%xmm1,%xmm3,%xmm12,%xmm1
[ 	]+168c:[ 	]+0f 24 26 62 14 04[ 	]+pcmov[ 	]+%xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+1692:[ 	]+0f 24 26 62 1c 04[ 	]+pcmov[ 	]+%xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+1698:[ 	]+0f 24 23 e3 14[ 	]+pperm[ 	]+%xmm3,%xmm12,%xmm1,%xmm1
[ 	]+169d:[ 	]+0f 24 23 62 14 04[ 	]+pperm[ 	]+0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+16a3:[ 	]+0f 24 23 62 1c 04[ 	]+pperm[ 	]+%xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+16a9:[ 	]+0f 24 27 e3 14[ 	]+pperm[ 	]+%xmm1,%xmm3,%xmm12,%xmm1
[ 	]+16ae:[ 	]+0f 24 27 62 14 04[ 	]+pperm[ 	]+%xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+16b4:[ 	]+0f 24 27 62 1c 04[ 	]+pperm[ 	]+%xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+16ba:[ 	]+0f 24 20 e3 14[ 	]+permps %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+16bf:[ 	]+0f 24 20 62 14 04[ 	]+permps 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+16c5:[ 	]+0f 24 20 62 1c 04[ 	]+permps %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+16cb:[ 	]+0f 24 24 e3 14[ 	]+permps %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+16d0:[ 	]+0f 24 24 62 14 04[ 	]+permps %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+16d6:[ 	]+0f 24 24 62 1c 04[ 	]+permps %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+16dc:[ 	]+0f 24 21 e3 14[ 	]+permpd %xmm3,%xmm12,%xmm1,%xmm1
[ 	]+16e1:[ 	]+0f 24 21 62 14 04[ 	]+permpd 0x4\(%rdx\),%xmm12,%xmm1,%xmm1
[ 	]+16e7:[ 	]+0f 24 21 62 1c 04[ 	]+permpd %xmm12,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+16ed:[ 	]+0f 24 25 e3 14[ 	]+permpd %xmm1,%xmm3,%xmm12,%xmm1
[ 	]+16f2:[ 	]+0f 24 25 62 14 04[ 	]+permpd %xmm1,0x4\(%rdx\),%xmm12,%xmm1
[ 	]+16f8:[ 	]+0f 24 25 62 1c 04[ 	]+permpd %xmm1,%xmm12,0x4\(%rdx\),%xmm1
[ 	]+16fe:[ 	]+0f 24 22 d3 b0[ 	]+pcmov[ 	]+%xmm3,%xmm2,%xmm11,%xmm11
[ 	]+1703:[ 	]+0f 24 22 52 b0 04[ 	]+pcmov[ 	]+0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+1709:[ 	]+0f 24 22 52 b8 04[ 	]+pcmov[ 	]+%xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+170f:[ 	]+0f 24 26 d3 b0[ 	]+pcmov[ 	]+%xmm11,%xmm3,%xmm2,%xmm11
[ 	]+1714:[ 	]+0f 24 26 52 b0 04[ 	]+pcmov[ 	]+%xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+171a:[ 	]+0f 24 26 52 b8 04[ 	]+pcmov[ 	]+%xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1720:[ 	]+0f 24 23 d3 b0[ 	]+pperm[ 	]+%xmm3,%xmm2,%xmm11,%xmm11
[ 	]+1725:[ 	]+0f 24 23 52 b0 04[ 	]+pperm[ 	]+0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+172b:[ 	]+0f 24 23 52 b8 04[ 	]+pperm[ 	]+%xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+1731:[ 	]+0f 24 27 d3 b0[ 	]+pperm[ 	]+%xmm11,%xmm3,%xmm2,%xmm11
[ 	]+1736:[ 	]+0f 24 27 52 b0 04[ 	]+pperm[ 	]+%xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+173c:[ 	]+0f 24 27 52 b8 04[ 	]+pperm[ 	]+%xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1742:[ 	]+0f 24 20 d3 b0[ 	]+permps %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+1747:[ 	]+0f 24 20 52 b0 04[ 	]+permps 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+174d:[ 	]+0f 24 20 52 b8 04[ 	]+permps %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+1753:[ 	]+0f 24 24 d3 b0[ 	]+permps %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+1758:[ 	]+0f 24 24 52 b0 04[ 	]+permps %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+175e:[ 	]+0f 24 24 52 b8 04[ 	]+permps %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1764:[ 	]+0f 24 21 d3 b0[ 	]+permpd %xmm3,%xmm2,%xmm11,%xmm11
[ 	]+1769:[ 	]+0f 24 21 52 b0 04[ 	]+permpd 0x4\(%rdx\),%xmm2,%xmm11,%xmm11
[ 	]+176f:[ 	]+0f 24 21 52 b8 04[ 	]+permpd %xmm2,0x4\(%rdx\),%xmm11,%xmm11
[ 	]+1775:[ 	]+0f 24 25 d3 b0[ 	]+permpd %xmm11,%xmm3,%xmm2,%xmm11
[ 	]+177a:[ 	]+0f 24 25 52 b0 04[ 	]+permpd %xmm11,0x4\(%rdx\),%xmm2,%xmm11
[ 	]+1780:[ 	]+0f 24 25 52 b8 04[ 	]+permpd %xmm11,%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1786:[ 	]+0f 24 22 d5 11[ 	]+pcmov[ 	]+%xmm13,%xmm2,%xmm1,%xmm1
[ 	]+178b:[ 	]+0f 24 22 52 10 04[ 	]+pcmov[ 	]+0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+1791:[ 	]+0f 24 22 52 18 04[ 	]+pcmov[ 	]+%xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+1797:[ 	]+0f 24 26 d5 11[ 	]+pcmov[ 	]+%xmm1,%xmm13,%xmm2,%xmm1
[ 	]+179c:[ 	]+0f 24 26 52 10 04[ 	]+pcmov[ 	]+%xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+17a2:[ 	]+0f 24 26 52 18 04[ 	]+pcmov[ 	]+%xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+17a8:[ 	]+0f 24 23 d5 11[ 	]+pperm[ 	]+%xmm13,%xmm2,%xmm1,%xmm1
[ 	]+17ad:[ 	]+0f 24 23 52 10 04[ 	]+pperm[ 	]+0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+17b3:[ 	]+0f 24 23 52 18 04[ 	]+pperm[ 	]+%xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+17b9:[ 	]+0f 24 27 d5 11[ 	]+pperm[ 	]+%xmm1,%xmm13,%xmm2,%xmm1
[ 	]+17be:[ 	]+0f 24 27 52 10 04[ 	]+pperm[ 	]+%xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+17c4:[ 	]+0f 24 27 52 18 04[ 	]+pperm[ 	]+%xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+17ca:[ 	]+0f 24 20 d5 11[ 	]+permps %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+17cf:[ 	]+0f 24 20 52 10 04[ 	]+permps 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+17d5:[ 	]+0f 24 20 52 18 04[ 	]+permps %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+17db:[ 	]+0f 24 24 d5 11[ 	]+permps %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+17e0:[ 	]+0f 24 24 52 10 04[ 	]+permps %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+17e6:[ 	]+0f 24 24 52 18 04[ 	]+permps %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+17ec:[ 	]+0f 24 21 d5 11[ 	]+permpd %xmm13,%xmm2,%xmm1,%xmm1
[ 	]+17f1:[ 	]+0f 24 21 52 10 04[ 	]+permpd 0x4\(%rdx\),%xmm2,%xmm1,%xmm1
[ 	]+17f7:[ 	]+0f 24 21 52 18 04[ 	]+permpd %xmm2,0x4\(%rdx\),%xmm1,%xmm1
[ 	]+17fd:[ 	]+0f 24 25 d5 11[ 	]+permpd %xmm1,%xmm13,%xmm2,%xmm1
[ 	]+1802:[ 	]+0f 24 25 52 10 04[ 	]+permpd %xmm1,0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1808:[ 	]+0f 24 25 52 18 04[ 	]+permpd %xmm1,%xmm2,0x4\(%rdx\),%xmm1
[ 	]+180e:[ 	]+0f 24 22 d3 10[ 	]+pcmov[ 	]+%xmm3,%xmm2,%xmm1,%xmm1
[ 	]+1813:[ 	]+0f 24 22 97 11 00 00 10 00[ 	]+pcmov[ 	]+0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+181c:[ 	]+0f 24 22 97 19 00 00 10 00[ 	]+pcmov[ 	]+%xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+1825:[ 	]+0f 24 26 d3 10[ 	]+pcmov[ 	]+%xmm1,%xmm3,%xmm2,%xmm1
[ 	]+182a:[ 	]+0f 24 26 97 11 00 00 10 00[ 	]+pcmov[ 	]+%xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+1833:[ 	]+0f 24 26 97 19 00 00 10 00[ 	]+pcmov[ 	]+%xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+183c:[ 	]+0f 24 23 d3 10[ 	]+pperm[ 	]+%xmm3,%xmm2,%xmm1,%xmm1
[ 	]+1841:[ 	]+0f 24 23 97 11 00 00 10 00[ 	]+pperm[ 	]+0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+184a:[ 	]+0f 24 23 97 19 00 00 10 00[ 	]+pperm[ 	]+%xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+1853:[ 	]+0f 24 27 d3 10[ 	]+pperm[ 	]+%xmm1,%xmm3,%xmm2,%xmm1
[ 	]+1858:[ 	]+0f 24 27 97 11 00 00 10 00[ 	]+pperm[ 	]+%xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+1861:[ 	]+0f 24 27 97 19 00 00 10 00[ 	]+pperm[ 	]+%xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+186a:[ 	]+0f 24 20 d3 10[ 	]+permps %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+186f:[ 	]+0f 24 20 97 11 00 00 10 00[ 	]+permps 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+1878:[ 	]+0f 24 20 97 19 00 00 10 00[ 	]+permps %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+1881:[ 	]+0f 24 24 d3 10[ 	]+permps %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+1886:[ 	]+0f 24 24 97 11 00 00 10 00[ 	]+permps %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+188f:[ 	]+0f 24 24 97 19 00 00 10 00[ 	]+permps %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+1898:[ 	]+0f 24 21 d3 10[ 	]+permpd %xmm3,%xmm2,%xmm1,%xmm1
[ 	]+189d:[ 	]+0f 24 21 97 11 00 00 10 00[ 	]+permpd 0x100000\(%r15\),%xmm2,%xmm1,%xmm1
[ 	]+18a6:[ 	]+0f 24 21 97 19 00 00 10 00[ 	]+permpd %xmm2,0x100000\(%r15\),%xmm1,%xmm1
[ 	]+18af:[ 	]+0f 24 25 d3 10[ 	]+permpd %xmm1,%xmm3,%xmm2,%xmm1
[ 	]+18b4:[ 	]+0f 24 25 97 11 00 00 10 00[ 	]+permpd %xmm1,0x100000\(%r15\),%xmm2,%xmm1
[ 	]+18bd:[ 	]+0f 24 25 97 19 00 00 10 00[ 	]+permpd %xmm1,%xmm2,0x100000\(%r15\),%xmm1
[ 	]+18c6:[ 	]+0f 24 40 d3 10[ 	]+protb[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+18cb:[ 	]+0f 24 40 52 10 04[ 	]+protb[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+18d1:[ 	]+0f 24 40 52 18 04[ 	]+protb[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+18d7:[ 	]+0f 7b 40 ca 04[ 	]+protb[ 	]+\$0x4,%xmm2,%xmm1
[ 	]+18dc:[ 	]+0f 24 41 d3 10[ 	]+protw[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+18e1:[ 	]+0f 24 41 52 10 04[ 	]+protw[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+18e7:[ 	]+0f 24 41 52 18 04[ 	]+protw[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+18ed:[ 	]+0f 7b 41 ca 04[ 	]+protw[ 	]+\$0x4,%xmm2,%xmm1
[ 	]+18f2:[ 	]+0f 24 42 d3 10[ 	]+protd[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+18f7:[ 	]+0f 24 42 52 10 04[ 	]+protd[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+18fd:[ 	]+0f 24 42 52 18 04[ 	]+protd[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1903:[ 	]+0f 7b 42 ca 04[ 	]+protd[ 	]+\$0x4,%xmm2,%xmm1
[ 	]+1908:[ 	]+0f 24 43 d3 10[ 	]+protq[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+190d:[ 	]+0f 24 43 52 10 04[ 	]+protq[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1913:[ 	]+0f 24 43 52 18 04[ 	]+protq[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1919:[ 	]+0f 7b 43 ca 04[ 	]+protq[ 	]+\$0x4,%xmm2,%xmm1
[ 	]+191e:[ 	]+0f 24 40 e5 b5[ 	]+protb[ 	]+%xmm13,%xmm12,%xmm11
[ 	]+1923:[ 	]+0f 24 40 a7 b5 00 00 10 00[ 	]+protb[ 	]+0x100000\(%r15\),%xmm12,%xmm11
[ 	]+192c:[ 	]+0f 24 40 a7 bd 00 00 10 00[ 	]+protb[ 	]+%xmm12,0x100000\(%r15\),%xmm11
[ 	]+1935:[ 	]+45 0f 7b 40 dc 04[ 	]+protb[ 	]+\$0x4,%xmm12,%xmm11
[ 	]+193b:[ 	]+0f 24 41 e5 b5[ 	]+protw[ 	]+%xmm13,%xmm12,%xmm11
[ 	]+1940:[ 	]+0f 24 41 a7 b5 00 00 10 00[ 	]+protw[ 	]+0x100000\(%r15\),%xmm12,%xmm11
[ 	]+1949:[ 	]+0f 24 41 a7 bd 00 00 10 00[ 	]+protw[ 	]+%xmm12,0x100000\(%r15\),%xmm11
[ 	]+1952:[ 	]+45 0f 7b 41 dc 04[ 	]+protw[ 	]+\$0x4,%xmm12,%xmm11
[ 	]+1958:[ 	]+0f 24 42 e5 b5[ 	]+protd[ 	]+%xmm13,%xmm12,%xmm11
[ 	]+195d:[ 	]+0f 24 42 a7 b5 00 00 10 00[ 	]+protd[ 	]+0x100000\(%r15\),%xmm12,%xmm11
[ 	]+1966:[ 	]+0f 24 42 a7 bd 00 00 10 00[ 	]+protd[ 	]+%xmm12,0x100000\(%r15\),%xmm11
[ 	]+196f:[ 	]+45 0f 7b 42 dc 04[ 	]+protd[ 	]+\$0x4,%xmm12,%xmm11
[ 	]+1975:[ 	]+0f 24 43 e5 b5[ 	]+protq[ 	]+%xmm13,%xmm12,%xmm11
[ 	]+197a:[ 	]+0f 24 43 a7 b5 00 00 10 00[ 	]+protq[ 	]+0x100000\(%r15\),%xmm12,%xmm11
[ 	]+1983:[ 	]+0f 24 43 a7 bd 00 00 10 00[ 	]+protq[ 	]+%xmm12,0x100000\(%r15\),%xmm11
[ 	]+198c:[ 	]+45 0f 7b 43 dc 04[ 	]+protq[ 	]+\$0x4,%xmm12,%xmm11
[ 	]+1992:[ 	]+0f 24 40 e3 14[ 	]+protb[ 	]+%xmm3,%xmm12,%xmm1
[ 	]+1997:[ 	]+0f 24 40 62 14 04[ 	]+protb[ 	]+0x4\(%rdx\),%xmm12,%xmm1
[ 	]+199d:[ 	]+0f 24 40 62 1c 04[ 	]+protb[ 	]+%xmm12,0x4\(%rdx\),%xmm1
[ 	]+19a3:[ 	]+41 0f 7b 40 cc 04[ 	]+protb[ 	]+\$0x4,%xmm12,%xmm1
[ 	]+19a9:[ 	]+0f 24 41 e3 14[ 	]+protw[ 	]+%xmm3,%xmm12,%xmm1
[ 	]+19ae:[ 	]+0f 24 41 62 14 04[ 	]+protw[ 	]+0x4\(%rdx\),%xmm12,%xmm1
[ 	]+19b4:[ 	]+0f 24 41 62 1c 04[ 	]+protw[ 	]+%xmm12,0x4\(%rdx\),%xmm1
[ 	]+19ba:[ 	]+41 0f 7b 41 cc 04[ 	]+protw[ 	]+\$0x4,%xmm12,%xmm1
[ 	]+19c0:[ 	]+0f 24 42 e3 14[ 	]+protd[ 	]+%xmm3,%xmm12,%xmm1
[ 	]+19c5:[ 	]+0f 24 42 62 14 04[ 	]+protd[ 	]+0x4\(%rdx\),%xmm12,%xmm1
[ 	]+19cb:[ 	]+0f 24 42 62 1c 04[ 	]+protd[ 	]+%xmm12,0x4\(%rdx\),%xmm1
[ 	]+19d1:[ 	]+41 0f 7b 42 cc 04[ 	]+protd[ 	]+\$0x4,%xmm12,%xmm1
[ 	]+19d7:[ 	]+0f 24 43 e3 14[ 	]+protq[ 	]+%xmm3,%xmm12,%xmm1
[ 	]+19dc:[ 	]+0f 24 43 62 14 04[ 	]+protq[ 	]+0x4\(%rdx\),%xmm12,%xmm1
[ 	]+19e2:[ 	]+0f 24 43 62 1c 04[ 	]+protq[ 	]+%xmm12,0x4\(%rdx\),%xmm1
[ 	]+19e8:[ 	]+41 0f 7b 43 cc 04[ 	]+protq[ 	]+\$0x4,%xmm12,%xmm1
[ 	]+19ee:[ 	]+0f 24 40 d3 b0[ 	]+protb[ 	]+%xmm3,%xmm2,%xmm11
[ 	]+19f3:[ 	]+0f 24 40 52 b0 04[ 	]+protb[ 	]+0x4\(%rdx\),%xmm2,%xmm11
[ 	]+19f9:[ 	]+0f 24 40 52 b8 04[ 	]+protb[ 	]+%xmm2,0x4\(%rdx\),%xmm11
[ 	]+19ff:[ 	]+44 0f 7b 40 da 04[ 	]+protb[ 	]+\$0x4,%xmm2,%xmm11
[ 	]+1a05:[ 	]+0f 24 41 d3 b0[ 	]+protw[ 	]+%xmm3,%xmm2,%xmm11
[ 	]+1a0a:[ 	]+0f 24 41 52 b0 04[ 	]+protw[ 	]+0x4\(%rdx\),%xmm2,%xmm11
[ 	]+1a10:[ 	]+0f 24 41 52 b8 04[ 	]+protw[ 	]+%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1a16:[ 	]+44 0f 7b 41 da 04[ 	]+protw[ 	]+\$0x4,%xmm2,%xmm11
[ 	]+1a1c:[ 	]+0f 24 42 d3 b0[ 	]+protd[ 	]+%xmm3,%xmm2,%xmm11
[ 	]+1a21:[ 	]+0f 24 42 52 b0 04[ 	]+protd[ 	]+0x4\(%rdx\),%xmm2,%xmm11
[ 	]+1a27:[ 	]+0f 24 42 52 b8 04[ 	]+protd[ 	]+%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1a2d:[ 	]+44 0f 7b 42 da 04[ 	]+protd[ 	]+\$0x4,%xmm2,%xmm11
[ 	]+1a33:[ 	]+0f 24 43 d3 b0[ 	]+protq[ 	]+%xmm3,%xmm2,%xmm11
[ 	]+1a38:[ 	]+0f 24 43 52 b0 04[ 	]+protq[ 	]+0x4\(%rdx\),%xmm2,%xmm11
[ 	]+1a3e:[ 	]+0f 24 43 52 b8 04[ 	]+protq[ 	]+%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1a44:[ 	]+44 0f 7b 43 da 04[ 	]+protq[ 	]+\$0x4,%xmm2,%xmm11
[ 	]+1a4a:[ 	]+0f 24 40 d5 11[ 	]+protb[ 	]+%xmm13,%xmm2,%xmm1
[ 	]+1a4f:[ 	]+0f 24 40 52 10 04[ 	]+protb[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1a55:[ 	]+0f 24 40 52 18 04[ 	]+protb[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1a5b:[ 	]+0f 7b 40 ca 04[ 	]+protb[ 	]+\$0x4,%xmm2,%xmm1
[ 	]+1a60:[ 	]+0f 24 41 d5 11[ 	]+protw[ 	]+%xmm13,%xmm2,%xmm1
[ 	]+1a65:[ 	]+0f 24 41 52 10 04[ 	]+protw[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1a6b:[ 	]+0f 24 41 52 18 04[ 	]+protw[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1a71:[ 	]+0f 7b 41 ca 04[ 	]+protw[ 	]+\$0x4,%xmm2,%xmm1
[ 	]+1a76:[ 	]+0f 24 42 d5 11[ 	]+protd[ 	]+%xmm13,%xmm2,%xmm1
[ 	]+1a7b:[ 	]+0f 24 42 52 10 04[ 	]+protd[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1a81:[ 	]+0f 24 42 52 18 04[ 	]+protd[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1a87:[ 	]+0f 7b 42 ca 04[ 	]+protd[ 	]+\$0x4,%xmm2,%xmm1
[ 	]+1a8c:[ 	]+0f 24 43 d5 11[ 	]+protq[ 	]+%xmm13,%xmm2,%xmm1
[ 	]+1a91:[ 	]+0f 24 43 52 10 04[ 	]+protq[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1a97:[ 	]+0f 24 43 52 18 04[ 	]+protq[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1a9d:[ 	]+0f 7b 43 ca 04[ 	]+protq[ 	]+\$0x4,%xmm2,%xmm1
[ 	]+1aa2:[ 	]+0f 24 40 d3 10[ 	]+protb[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1aa7:[ 	]+0f 24 40 97 11 00 00 10 00[ 	]+protb[ 	]+0x100000\(%r15\),%xmm2,%xmm1
[ 	]+1ab0:[ 	]+0f 24 40 97 19 00 00 10 00[ 	]+protb[ 	]+%xmm2,0x100000\(%r15\),%xmm1
[ 	]+1ab9:[ 	]+0f 7b 40 ca 04[ 	]+protb[ 	]+\$0x4,%xmm2,%xmm1
[ 	]+1abe:[ 	]+0f 24 41 d3 10[ 	]+protw[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1ac3:[ 	]+0f 24 41 97 11 00 00 10 00[ 	]+protw[ 	]+0x100000\(%r15\),%xmm2,%xmm1
[ 	]+1acc:[ 	]+0f 24 41 97 19 00 00 10 00[ 	]+protw[ 	]+%xmm2,0x100000\(%r15\),%xmm1
[ 	]+1ad5:[ 	]+0f 7b 41 ca 04[ 	]+protw[ 	]+\$0x4,%xmm2,%xmm1
[ 	]+1ada:[ 	]+0f 24 42 d3 10[ 	]+protd[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1adf:[ 	]+0f 24 42 97 11 00 00 10 00[ 	]+protd[ 	]+0x100000\(%r15\),%xmm2,%xmm1
[ 	]+1ae8:[ 	]+0f 24 42 97 19 00 00 10 00[ 	]+protd[ 	]+%xmm2,0x100000\(%r15\),%xmm1
[ 	]+1af1:[ 	]+0f 7b 42 ca 04[ 	]+protd[ 	]+\$0x4,%xmm2,%xmm1
[ 	]+1af6:[ 	]+0f 24 43 d3 10[ 	]+protq[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1afb:[ 	]+0f 24 43 97 11 00 00 10 00[ 	]+protq[ 	]+0x100000\(%r15\),%xmm2,%xmm1
[ 	]+1b04:[ 	]+0f 24 43 97 19 00 00 10 00[ 	]+protq[ 	]+%xmm2,0x100000\(%r15\),%xmm1
[ 	]+1b0d:[ 	]+0f 7b 43 ca 04[ 	]+protq[ 	]+\$0x4,%xmm2,%xmm1
[ 	]+1b12:[ 	]+0f 24 44 d3 10[ 	]+pshlb[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1b17:[ 	]+0f 24 44 52 10 04[ 	]+pshlb[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1b1d:[ 	]+0f 24 44 52 18 04[ 	]+pshlb[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1b23:[ 	]+0f 24 45 d3 10[ 	]+pshlw[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1b28:[ 	]+0f 24 45 52 10 04[ 	]+pshlw[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1b2e:[ 	]+0f 24 45 52 18 04[ 	]+pshlw[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1b34:[ 	]+0f 24 46 d3 10[ 	]+pshld[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1b39:[ 	]+0f 24 46 52 10 04[ 	]+pshld[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1b3f:[ 	]+0f 24 46 52 18 04[ 	]+pshld[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1b45:[ 	]+0f 24 47 d3 10[ 	]+pshlq[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1b4a:[ 	]+0f 24 47 52 10 04[ 	]+pshlq[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1b50:[ 	]+0f 24 47 52 18 04[ 	]+pshlq[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1b56:[ 	]+0f 24 48 d3 10[ 	]+pshab[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1b5b:[ 	]+0f 24 48 52 10 04[ 	]+pshab[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1b61:[ 	]+0f 24 48 52 18 04[ 	]+pshab[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1b67:[ 	]+0f 24 49 d3 10[ 	]+pshaw[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1b6c:[ 	]+0f 24 49 52 10 04[ 	]+pshaw[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1b72:[ 	]+0f 24 49 52 18 04[ 	]+pshaw[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1b78:[ 	]+0f 24 4a d3 10[ 	]+pshad[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1b7d:[ 	]+0f 24 4a 52 10 04[ 	]+pshad[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1b83:[ 	]+0f 24 4a 52 18 04[ 	]+pshad[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1b89:[ 	]+0f 24 4b d3 10[ 	]+pshaq[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1b8e:[ 	]+0f 24 4b 52 10 04[ 	]+pshaq[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1b94:[ 	]+0f 24 4b 52 18 04[ 	]+pshaq[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1b9a:[ 	]+0f 24 44 e5 b5[ 	]+pshlb[ 	]+%xmm13,%xmm12,%xmm11
[ 	]+1b9f:[ 	]+0f 24 44 a7 b5 00 00 10 00[ 	]+pshlb[ 	]+0x100000\(%r15\),%xmm12,%xmm11
[ 	]+1ba8:[ 	]+0f 24 44 a7 bd 00 00 10 00[ 	]+pshlb[ 	]+%xmm12,0x100000\(%r15\),%xmm11
[ 	]+1bb1:[ 	]+0f 24 45 e5 b5[ 	]+pshlw[ 	]+%xmm13,%xmm12,%xmm11
[ 	]+1bb6:[ 	]+0f 24 45 a7 b5 00 00 10 00[ 	]+pshlw[ 	]+0x100000\(%r15\),%xmm12,%xmm11
[ 	]+1bbf:[ 	]+0f 24 45 a7 bd 00 00 10 00[ 	]+pshlw[ 	]+%xmm12,0x100000\(%r15\),%xmm11
[ 	]+1bc8:[ 	]+0f 24 46 e5 b5[ 	]+pshld[ 	]+%xmm13,%xmm12,%xmm11
[ 	]+1bcd:[ 	]+0f 24 46 a7 b5 00 00 10 00[ 	]+pshld[ 	]+0x100000\(%r15\),%xmm12,%xmm11
[ 	]+1bd6:[ 	]+0f 24 46 a7 bd 00 00 10 00[ 	]+pshld[ 	]+%xmm12,0x100000\(%r15\),%xmm11
[ 	]+1bdf:[ 	]+0f 24 47 e5 b5[ 	]+pshlq[ 	]+%xmm13,%xmm12,%xmm11
[ 	]+1be4:[ 	]+0f 24 47 a7 b5 00 00 10 00[ 	]+pshlq[ 	]+0x100000\(%r15\),%xmm12,%xmm11
[ 	]+1bed:[ 	]+0f 24 47 a7 bd 00 00 10 00[ 	]+pshlq[ 	]+%xmm12,0x100000\(%r15\),%xmm11
[ 	]+1bf6:[ 	]+0f 24 48 e5 b5[ 	]+pshab[ 	]+%xmm13,%xmm12,%xmm11
[ 	]+1bfb:[ 	]+0f 24 48 a7 b5 00 00 10 00[ 	]+pshab[ 	]+0x100000\(%r15\),%xmm12,%xmm11
[ 	]+1c04:[ 	]+0f 24 48 a7 bd 00 00 10 00[ 	]+pshab[ 	]+%xmm12,0x100000\(%r15\),%xmm11
[ 	]+1c0d:[ 	]+0f 24 49 e5 b5[ 	]+pshaw[ 	]+%xmm13,%xmm12,%xmm11
[ 	]+1c12:[ 	]+0f 24 49 a7 b5 00 00 10 00[ 	]+pshaw[ 	]+0x100000\(%r15\),%xmm12,%xmm11
[ 	]+1c1b:[ 	]+0f 24 49 a7 bd 00 00 10 00[ 	]+pshaw[ 	]+%xmm12,0x100000\(%r15\),%xmm11
[ 	]+1c24:[ 	]+0f 24 4a e5 b5[ 	]+pshad[ 	]+%xmm13,%xmm12,%xmm11
[ 	]+1c29:[ 	]+0f 24 4a a7 b5 00 00 10 00[ 	]+pshad[ 	]+0x100000\(%r15\),%xmm12,%xmm11
[ 	]+1c32:[ 	]+0f 24 4a a7 bd 00 00 10 00[ 	]+pshad[ 	]+%xmm12,0x100000\(%r15\),%xmm11
[ 	]+1c3b:[ 	]+0f 24 4b e5 b5[ 	]+pshaq[ 	]+%xmm13,%xmm12,%xmm11
[ 	]+1c40:[ 	]+0f 24 4b a7 b5 00 00 10 00[ 	]+pshaq[ 	]+0x100000\(%r15\),%xmm12,%xmm11
[ 	]+1c49:[ 	]+0f 24 4b a7 bd 00 00 10 00[ 	]+pshaq[ 	]+%xmm12,0x100000\(%r15\),%xmm11
[ 	]+1c52:[ 	]+0f 24 44 e3 14[ 	]+pshlb[ 	]+%xmm3,%xmm12,%xmm1
[ 	]+1c57:[ 	]+0f 24 44 62 14 04[ 	]+pshlb[ 	]+0x4\(%rdx\),%xmm12,%xmm1
[ 	]+1c5d:[ 	]+0f 24 44 62 1c 04[ 	]+pshlb[ 	]+%xmm12,0x4\(%rdx\),%xmm1
[ 	]+1c63:[ 	]+0f 24 45 e3 14[ 	]+pshlw[ 	]+%xmm3,%xmm12,%xmm1
[ 	]+1c68:[ 	]+0f 24 45 62 14 04[ 	]+pshlw[ 	]+0x4\(%rdx\),%xmm12,%xmm1
[ 	]+1c6e:[ 	]+0f 24 45 62 1c 04[ 	]+pshlw[ 	]+%xmm12,0x4\(%rdx\),%xmm1
[ 	]+1c74:[ 	]+0f 24 46 e3 14[ 	]+pshld[ 	]+%xmm3,%xmm12,%xmm1
[ 	]+1c79:[ 	]+0f 24 46 62 14 04[ 	]+pshld[ 	]+0x4\(%rdx\),%xmm12,%xmm1
[ 	]+1c7f:[ 	]+0f 24 46 62 1c 04[ 	]+pshld[ 	]+%xmm12,0x4\(%rdx\),%xmm1
[ 	]+1c85:[ 	]+0f 24 47 e3 14[ 	]+pshlq[ 	]+%xmm3,%xmm12,%xmm1
[ 	]+1c8a:[ 	]+0f 24 47 62 14 04[ 	]+pshlq[ 	]+0x4\(%rdx\),%xmm12,%xmm1
[ 	]+1c90:[ 	]+0f 24 47 62 1c 04[ 	]+pshlq[ 	]+%xmm12,0x4\(%rdx\),%xmm1
[ 	]+1c96:[ 	]+0f 24 48 e3 14[ 	]+pshab[ 	]+%xmm3,%xmm12,%xmm1
[ 	]+1c9b:[ 	]+0f 24 48 62 14 04[ 	]+pshab[ 	]+0x4\(%rdx\),%xmm12,%xmm1
[ 	]+1ca1:[ 	]+0f 24 48 62 1c 04[ 	]+pshab[ 	]+%xmm12,0x4\(%rdx\),%xmm1
[ 	]+1ca7:[ 	]+0f 24 49 e3 14[ 	]+pshaw[ 	]+%xmm3,%xmm12,%xmm1
[ 	]+1cac:[ 	]+0f 24 49 62 14 04[ 	]+pshaw[ 	]+0x4\(%rdx\),%xmm12,%xmm1
[ 	]+1cb2:[ 	]+0f 24 49 62 1c 04[ 	]+pshaw[ 	]+%xmm12,0x4\(%rdx\),%xmm1
[ 	]+1cb8:[ 	]+0f 24 4a e3 14[ 	]+pshad[ 	]+%xmm3,%xmm12,%xmm1
[ 	]+1cbd:[ 	]+0f 24 4a 62 14 04[ 	]+pshad[ 	]+0x4\(%rdx\),%xmm12,%xmm1
[ 	]+1cc3:[ 	]+0f 24 4a 62 1c 04[ 	]+pshad[ 	]+%xmm12,0x4\(%rdx\),%xmm1
[ 	]+1cc9:[ 	]+0f 24 4b e3 14[ 	]+pshaq[ 	]+%xmm3,%xmm12,%xmm1
[ 	]+1cce:[ 	]+0f 24 4b 62 14 04[ 	]+pshaq[ 	]+0x4\(%rdx\),%xmm12,%xmm1
[ 	]+1cd4:[ 	]+0f 24 4b 62 1c 04[ 	]+pshaq[ 	]+%xmm12,0x4\(%rdx\),%xmm1
[ 	]+1cda:[ 	]+0f 24 44 d3 b0[ 	]+pshlb[ 	]+%xmm3,%xmm2,%xmm11
[ 	]+1cdf:[ 	]+0f 24 44 52 b0 04[ 	]+pshlb[ 	]+0x4\(%rdx\),%xmm2,%xmm11
[ 	]+1ce5:[ 	]+0f 24 44 52 b8 04[ 	]+pshlb[ 	]+%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1ceb:[ 	]+0f 24 45 d3 b0[ 	]+pshlw[ 	]+%xmm3,%xmm2,%xmm11
[ 	]+1cf0:[ 	]+0f 24 45 52 b0 04[ 	]+pshlw[ 	]+0x4\(%rdx\),%xmm2,%xmm11
[ 	]+1cf6:[ 	]+0f 24 45 52 b8 04[ 	]+pshlw[ 	]+%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1cfc:[ 	]+0f 24 46 d3 b0[ 	]+pshld[ 	]+%xmm3,%xmm2,%xmm11
[ 	]+1d01:[ 	]+0f 24 46 52 b0 04[ 	]+pshld[ 	]+0x4\(%rdx\),%xmm2,%xmm11
[ 	]+1d07:[ 	]+0f 24 46 52 b8 04[ 	]+pshld[ 	]+%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1d0d:[ 	]+0f 24 47 d3 b0[ 	]+pshlq[ 	]+%xmm3,%xmm2,%xmm11
[ 	]+1d12:[ 	]+0f 24 47 52 b0 04[ 	]+pshlq[ 	]+0x4\(%rdx\),%xmm2,%xmm11
[ 	]+1d18:[ 	]+0f 24 47 52 b8 04[ 	]+pshlq[ 	]+%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1d1e:[ 	]+0f 24 48 d3 b0[ 	]+pshab[ 	]+%xmm3,%xmm2,%xmm11
[ 	]+1d23:[ 	]+0f 24 48 52 b0 04[ 	]+pshab[ 	]+0x4\(%rdx\),%xmm2,%xmm11
[ 	]+1d29:[ 	]+0f 24 48 52 b8 04[ 	]+pshab[ 	]+%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1d2f:[ 	]+0f 24 49 d3 b0[ 	]+pshaw[ 	]+%xmm3,%xmm2,%xmm11
[ 	]+1d34:[ 	]+0f 24 49 52 b0 04[ 	]+pshaw[ 	]+0x4\(%rdx\),%xmm2,%xmm11
[ 	]+1d3a:[ 	]+0f 24 49 52 b8 04[ 	]+pshaw[ 	]+%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1d40:[ 	]+0f 24 4a d3 b0[ 	]+pshad[ 	]+%xmm3,%xmm2,%xmm11
[ 	]+1d45:[ 	]+0f 24 4a 52 b0 04[ 	]+pshad[ 	]+0x4\(%rdx\),%xmm2,%xmm11
[ 	]+1d4b:[ 	]+0f 24 4a 52 b8 04[ 	]+pshad[ 	]+%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1d51:[ 	]+0f 24 4b d3 b0[ 	]+pshaq[ 	]+%xmm3,%xmm2,%xmm11
[ 	]+1d56:[ 	]+0f 24 4b 52 b0 04[ 	]+pshaq[ 	]+0x4\(%rdx\),%xmm2,%xmm11
[ 	]+1d5c:[ 	]+0f 24 4b 52 b8 04[ 	]+pshaq[ 	]+%xmm2,0x4\(%rdx\),%xmm11
[ 	]+1d62:[ 	]+0f 24 44 d5 11[ 	]+pshlb[ 	]+%xmm13,%xmm2,%xmm1
[ 	]+1d67:[ 	]+0f 24 44 52 10 04[ 	]+pshlb[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1d6d:[ 	]+0f 24 44 52 18 04[ 	]+pshlb[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1d73:[ 	]+0f 24 45 d5 11[ 	]+pshlw[ 	]+%xmm13,%xmm2,%xmm1
[ 	]+1d78:[ 	]+0f 24 45 52 10 04[ 	]+pshlw[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1d7e:[ 	]+0f 24 45 52 18 04[ 	]+pshlw[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1d84:[ 	]+0f 24 46 d5 11[ 	]+pshld[ 	]+%xmm13,%xmm2,%xmm1
[ 	]+1d89:[ 	]+0f 24 46 52 10 04[ 	]+pshld[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1d8f:[ 	]+0f 24 46 52 18 04[ 	]+pshld[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1d95:[ 	]+0f 24 47 d5 11[ 	]+pshlq[ 	]+%xmm13,%xmm2,%xmm1
[ 	]+1d9a:[ 	]+0f 24 47 52 10 04[ 	]+pshlq[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1da0:[ 	]+0f 24 47 52 18 04[ 	]+pshlq[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1da6:[ 	]+0f 24 48 d5 11[ 	]+pshab[ 	]+%xmm13,%xmm2,%xmm1
[ 	]+1dab:[ 	]+0f 24 48 52 10 04[ 	]+pshab[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1db1:[ 	]+0f 24 48 52 18 04[ 	]+pshab[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1db7:[ 	]+0f 24 49 d5 11[ 	]+pshaw[ 	]+%xmm13,%xmm2,%xmm1
[ 	]+1dbc:[ 	]+0f 24 49 52 10 04[ 	]+pshaw[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1dc2:[ 	]+0f 24 49 52 18 04[ 	]+pshaw[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1dc8:[ 	]+0f 24 4a d5 11[ 	]+pshad[ 	]+%xmm13,%xmm2,%xmm1
[ 	]+1dcd:[ 	]+0f 24 4a 52 10 04[ 	]+pshad[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1dd3:[ 	]+0f 24 4a 52 18 04[ 	]+pshad[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1dd9:[ 	]+0f 24 4b d5 11[ 	]+pshaq[ 	]+%xmm13,%xmm2,%xmm1
[ 	]+1dde:[ 	]+0f 24 4b 52 10 04[ 	]+pshaq[ 	]+0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1de4:[ 	]+0f 24 4b 52 18 04[ 	]+pshaq[ 	]+%xmm2,0x4\(%rdx\),%xmm1
[ 	]+1dea:[ 	]+0f 24 44 d3 10[ 	]+pshlb[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1def:[ 	]+0f 24 44 97 11 00 00 10 00[ 	]+pshlb[ 	]+0x100000\(%r15\),%xmm2,%xmm1
[ 	]+1df8:[ 	]+0f 24 44 97 19 00 00 10 00[ 	]+pshlb[ 	]+%xmm2,0x100000\(%r15\),%xmm1
[ 	]+1e01:[ 	]+0f 24 45 d3 10[ 	]+pshlw[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1e06:[ 	]+0f 24 45 97 11 00 00 10 00[ 	]+pshlw[ 	]+0x100000\(%r15\),%xmm2,%xmm1
[ 	]+1e0f:[ 	]+0f 24 45 97 19 00 00 10 00[ 	]+pshlw[ 	]+%xmm2,0x100000\(%r15\),%xmm1
[ 	]+1e18:[ 	]+0f 24 46 d3 10[ 	]+pshld[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1e1d:[ 	]+0f 24 46 97 11 00 00 10 00[ 	]+pshld[ 	]+0x100000\(%r15\),%xmm2,%xmm1
[ 	]+1e26:[ 	]+0f 24 46 97 19 00 00 10 00[ 	]+pshld[ 	]+%xmm2,0x100000\(%r15\),%xmm1
[ 	]+1e2f:[ 	]+0f 24 47 d3 10[ 	]+pshlq[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1e34:[ 	]+0f 24 47 97 11 00 00 10 00[ 	]+pshlq[ 	]+0x100000\(%r15\),%xmm2,%xmm1
[ 	]+1e3d:[ 	]+0f 24 47 97 19 00 00 10 00[ 	]+pshlq[ 	]+%xmm2,0x100000\(%r15\),%xmm1
[ 	]+1e46:[ 	]+0f 24 48 d3 10[ 	]+pshab[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1e4b:[ 	]+0f 24 48 97 11 00 00 10 00[ 	]+pshab[ 	]+0x100000\(%r15\),%xmm2,%xmm1
[ 	]+1e54:[ 	]+0f 24 48 97 19 00 00 10 00[ 	]+pshab[ 	]+%xmm2,0x100000\(%r15\),%xmm1
[ 	]+1e5d:[ 	]+0f 24 49 d3 10[ 	]+pshaw[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1e62:[ 	]+0f 24 49 97 11 00 00 10 00[ 	]+pshaw[ 	]+0x100000\(%r15\),%xmm2,%xmm1
[ 	]+1e6b:[ 	]+0f 24 49 97 19 00 00 10 00[ 	]+pshaw[ 	]+%xmm2,0x100000\(%r15\),%xmm1
[ 	]+1e74:[ 	]+0f 24 4a d3 10[ 	]+pshad[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1e79:[ 	]+0f 24 4a 97 11 00 00 10 00[ 	]+pshad[ 	]+0x100000\(%r15\),%xmm2,%xmm1
[ 	]+1e82:[ 	]+0f 24 4a 97 19 00 00 10 00[ 	]+pshad[ 	]+%xmm2,0x100000\(%r15\),%xmm1
[ 	]+1e8b:[ 	]+0f 24 4b d3 10[ 	]+pshaq[ 	]+%xmm3,%xmm2,%xmm1
[ 	]+1e90:[ 	]+0f 24 4b 97 11 00 00 10 00[ 	]+pshaq[ 	]+0x100000\(%r15\),%xmm2,%xmm1
[ 	]+1e99:[ 	]+0f 24 4b 97 19 00 00 10 00[ 	]+pshaq[ 	]+%xmm2,0x100000\(%r15\),%xmm1
[ 	]+1ea2:[ 	]+0f 25 2e d3 10 04[ 	]+comness %xmm3,%xmm2,%xmm1
[ 	]+1ea8:[ 	]+0f 25 2e 52 10 04 04[ 	]+comness 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1eaf:[ 	]+0f 25 2e d3 10 00[ 	]+comeqss %xmm3,%xmm2,%xmm1
[ 	]+1eb5:[ 	]+0f 25 2e d3 10 01[ 	]+comltss %xmm3,%xmm2,%xmm1
[ 	]+1ebb:[ 	]+0f 25 2e d3 10 02[ 	]+comless %xmm3,%xmm2,%xmm1
[ 	]+1ec1:[ 	]+0f 25 2e d3 10 03[ 	]+comunordss %xmm3,%xmm2,%xmm1
[ 	]+1ec7:[ 	]+0f 25 2e d3 10 04[ 	]+comness %xmm3,%xmm2,%xmm1
[ 	]+1ecd:[ 	]+0f 25 2e d3 10 05[ 	]+comnltss %xmm3,%xmm2,%xmm1
[ 	]+1ed3:[ 	]+0f 25 2e d3 10 06[ 	]+comnless %xmm3,%xmm2,%xmm1
[ 	]+1ed9:[ 	]+0f 25 2e d3 10 07[ 	]+comordss %xmm3,%xmm2,%xmm1
[ 	]+1edf:[ 	]+0f 25 2e d3 10 08[ 	]+comueqss %xmm3,%xmm2,%xmm1
[ 	]+1ee5:[ 	]+0f 25 2e d3 10 09[ 	]+comultss %xmm3,%xmm2,%xmm1
[ 	]+1eeb:[ 	]+0f 25 2e d3 10 0a[ 	]+comuless %xmm3,%xmm2,%xmm1
[ 	]+1ef1:[ 	]+0f 25 2e d3 10 0b[ 	]+comfalsess %xmm3,%xmm2,%xmm1
[ 	]+1ef7:[ 	]+0f 25 2e d3 10 0c[ 	]+comuness %xmm3,%xmm2,%xmm1
[ 	]+1efd:[ 	]+0f 25 2e d3 10 0d[ 	]+comunltss %xmm3,%xmm2,%xmm1
[ 	]+1f03:[ 	]+0f 25 2e d3 10 0e[ 	]+comunless %xmm3,%xmm2,%xmm1
[ 	]+1f09:[ 	]+0f 25 2e d3 10 0f[ 	]+comtruess %xmm3,%xmm2,%xmm1
[ 	]+1f0f:[ 	]+0f 25 2e 52 10 04 00[ 	]+comeqss 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f16:[ 	]+0f 25 2e 52 10 04 01[ 	]+comltss 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f1d:[ 	]+0f 25 2e 52 10 04 02[ 	]+comless 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f24:[ 	]+0f 25 2e 52 10 04 03[ 	]+comunordss 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f2b:[ 	]+0f 25 2e 52 10 04 04[ 	]+comness 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f32:[ 	]+0f 25 2e 52 10 04 05[ 	]+comnltss 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f39:[ 	]+0f 25 2e 52 10 04 06[ 	]+comnless 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f40:[ 	]+0f 25 2e 52 10 04 07[ 	]+comordss 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f47:[ 	]+0f 25 2e 52 10 04 08[ 	]+comueqss 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f4e:[ 	]+0f 25 2e 52 10 04 09[ 	]+comultss 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f55:[ 	]+0f 25 2e 52 10 04 0a[ 	]+comuless 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f5c:[ 	]+0f 25 2e 52 10 04 0b[ 	]+comfalsess 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f63:[ 	]+0f 25 2e 52 10 04 0c[ 	]+comuness 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f6a:[ 	]+0f 25 2e 52 10 04 0d[ 	]+comunltss 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f71:[ 	]+0f 25 2e 52 10 04 0e[ 	]+comunless 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f78:[ 	]+0f 25 2e 52 10 04 0f[ 	]+comtruess 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f7f:[ 	]+0f 25 2f d3 10 04[ 	]+comnesd %xmm3,%xmm2,%xmm1
[ 	]+1f85:[ 	]+0f 25 2f 52 10 04 04[ 	]+comnesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1f8c:[ 	]+0f 25 2f d3 10 00[ 	]+comeqsd %xmm3,%xmm2,%xmm1
[ 	]+1f92:[ 	]+0f 25 2f d3 10 01[ 	]+comltsd %xmm3,%xmm2,%xmm1
[ 	]+1f98:[ 	]+0f 25 2f d3 10 02[ 	]+comlesd %xmm3,%xmm2,%xmm1
[ 	]+1f9e:[ 	]+0f 25 2f d3 10 03[ 	]+comunordsd %xmm3,%xmm2,%xmm1
[ 	]+1fa4:[ 	]+0f 25 2f d3 10 04[ 	]+comnesd %xmm3,%xmm2,%xmm1
[ 	]+1faa:[ 	]+0f 25 2f d3 10 05[ 	]+comnltsd %xmm3,%xmm2,%xmm1
[ 	]+1fb0:[ 	]+0f 25 2f d3 10 06[ 	]+comnlesd %xmm3,%xmm2,%xmm1
[ 	]+1fb6:[ 	]+0f 25 2f d3 10 07[ 	]+comordsd %xmm3,%xmm2,%xmm1
[ 	]+1fbc:[ 	]+0f 25 2f d3 10 08[ 	]+comueqsd %xmm3,%xmm2,%xmm1
[ 	]+1fc2:[ 	]+0f 25 2f d3 10 09[ 	]+comultsd %xmm3,%xmm2,%xmm1
[ 	]+1fc8:[ 	]+0f 25 2f d3 10 0a[ 	]+comulesd %xmm3,%xmm2,%xmm1
[ 	]+1fce:[ 	]+0f 25 2f d3 10 0b[ 	]+comfalsesd %xmm3,%xmm2,%xmm1
[ 	]+1fd4:[ 	]+0f 25 2f d3 10 0c[ 	]+comunesd %xmm3,%xmm2,%xmm1
[ 	]+1fda:[ 	]+0f 25 2f d3 10 0d[ 	]+comunltsd %xmm3,%xmm2,%xmm1
[ 	]+1fe0:[ 	]+0f 25 2f d3 10 0e[ 	]+comunlesd %xmm3,%xmm2,%xmm1
[ 	]+1fe6:[ 	]+0f 25 2f d3 10 0f[ 	]+comtruesd %xmm3,%xmm2,%xmm1
[ 	]+1fec:[ 	]+0f 25 2f 52 10 04 00[ 	]+comeqsd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1ff3:[ 	]+0f 25 2f 52 10 04 01[ 	]+comltsd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+1ffa:[ 	]+0f 25 2f 52 10 04 02[ 	]+comlesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2001:[ 	]+0f 25 2f 52 10 04 03[ 	]+comunordsd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2008:[ 	]+0f 25 2f 52 10 04 04[ 	]+comnesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+200f:[ 	]+0f 25 2f 52 10 04 05[ 	]+comnltsd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2016:[ 	]+0f 25 2f 52 10 04 06[ 	]+comnlesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+201d:[ 	]+0f 25 2f 52 10 04 07[ 	]+comordsd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2024:[ 	]+0f 25 2f 52 10 04 08[ 	]+comueqsd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+202b:[ 	]+0f 25 2f 52 10 04 09[ 	]+comultsd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2032:[ 	]+0f 25 2f 52 10 04 0a[ 	]+comulesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2039:[ 	]+0f 25 2f 52 10 04 0b[ 	]+comfalsesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2040:[ 	]+0f 25 2f 52 10 04 0c[ 	]+comunesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2047:[ 	]+0f 25 2f 52 10 04 0d[ 	]+comunltsd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+204e:[ 	]+0f 25 2f 52 10 04 0e[ 	]+comunlesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2055:[ 	]+0f 25 2f 52 10 04 0f[ 	]+comtruesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+205c:[ 	]+0f 25 2c d3 10 04[ 	]+comneps %xmm3,%xmm2,%xmm1
[ 	]+2062:[ 	]+0f 25 2c 52 10 04 04[ 	]+comneps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2069:[ 	]+0f 25 2c d3 10 00[ 	]+comeqps %xmm3,%xmm2,%xmm1
[ 	]+206f:[ 	]+0f 25 2c d3 10 01[ 	]+comltps %xmm3,%xmm2,%xmm1
[ 	]+2075:[ 	]+0f 25 2c d3 10 02[ 	]+comleps %xmm3,%xmm2,%xmm1
[ 	]+207b:[ 	]+0f 25 2c d3 10 03[ 	]+comunordps %xmm3,%xmm2,%xmm1
[ 	]+2081:[ 	]+0f 25 2c d3 10 04[ 	]+comneps %xmm3,%xmm2,%xmm1
[ 	]+2087:[ 	]+0f 25 2c d3 10 05[ 	]+comnltps %xmm3,%xmm2,%xmm1
[ 	]+208d:[ 	]+0f 25 2c d3 10 06[ 	]+comnleps %xmm3,%xmm2,%xmm1
[ 	]+2093:[ 	]+0f 25 2c d3 10 07[ 	]+comordps %xmm3,%xmm2,%xmm1
[ 	]+2099:[ 	]+0f 25 2c d3 10 08[ 	]+comueqps %xmm3,%xmm2,%xmm1
[ 	]+209f:[ 	]+0f 25 2c d3 10 09[ 	]+comultps %xmm3,%xmm2,%xmm1
[ 	]+20a5:[ 	]+0f 25 2c d3 10 0a[ 	]+comuleps %xmm3,%xmm2,%xmm1
[ 	]+20ab:[ 	]+0f 25 2c d3 10 0b[ 	]+comfalseps %xmm3,%xmm2,%xmm1
[ 	]+20b1:[ 	]+0f 25 2c d3 10 0c[ 	]+comuneps %xmm3,%xmm2,%xmm1
[ 	]+20b7:[ 	]+0f 25 2c d3 10 0d[ 	]+comunltps %xmm3,%xmm2,%xmm1
[ 	]+20bd:[ 	]+0f 25 2c d3 10 0e[ 	]+comunleps %xmm3,%xmm2,%xmm1
[ 	]+20c3:[ 	]+0f 25 2c d3 10 0f[ 	]+comtrueps %xmm3,%xmm2,%xmm1
[ 	]+20c9:[ 	]+0f 25 2c 52 10 04 00[ 	]+comeqps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+20d0:[ 	]+0f 25 2c 52 10 04 01[ 	]+comltps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+20d7:[ 	]+0f 25 2c 52 10 04 02[ 	]+comleps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+20de:[ 	]+0f 25 2c 52 10 04 03[ 	]+comunordps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+20e5:[ 	]+0f 25 2c 52 10 04 04[ 	]+comneps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+20ec:[ 	]+0f 25 2c 52 10 04 05[ 	]+comnltps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+20f3:[ 	]+0f 25 2c 52 10 04 06[ 	]+comnleps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+20fa:[ 	]+0f 25 2c 52 10 04 07[ 	]+comordps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2101:[ 	]+0f 25 2c 52 10 04 08[ 	]+comueqps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2108:[ 	]+0f 25 2c 52 10 04 09[ 	]+comultps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+210f:[ 	]+0f 25 2c 52 10 04 0a[ 	]+comuleps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2116:[ 	]+0f 25 2c 52 10 04 0b[ 	]+comfalseps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+211d:[ 	]+0f 25 2c 52 10 04 0c[ 	]+comuneps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2124:[ 	]+0f 25 2c 52 10 04 0d[ 	]+comunltps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+212b:[ 	]+0f 25 2c 52 10 04 0e[ 	]+comunleps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2132:[ 	]+0f 25 2c 52 10 04 0f[ 	]+comtrueps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2139:[ 	]+0f 25 2d d3 10 04[ 	]+comnepd %xmm3,%xmm2,%xmm1
[ 	]+213f:[ 	]+0f 25 2d 52 10 04 04[ 	]+comnepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2146:[ 	]+0f 25 2d d3 10 00[ 	]+comeqpd %xmm3,%xmm2,%xmm1
[ 	]+214c:[ 	]+0f 25 2d d3 10 01[ 	]+comltpd %xmm3,%xmm2,%xmm1
[ 	]+2152:[ 	]+0f 25 2d d3 10 02[ 	]+comlepd %xmm3,%xmm2,%xmm1
[ 	]+2158:[ 	]+0f 25 2d d3 10 03[ 	]+comunordpd %xmm3,%xmm2,%xmm1
[ 	]+215e:[ 	]+0f 25 2d d3 10 04[ 	]+comnepd %xmm3,%xmm2,%xmm1
[ 	]+2164:[ 	]+0f 25 2d d3 10 05[ 	]+comnltpd %xmm3,%xmm2,%xmm1
[ 	]+216a:[ 	]+0f 25 2d d3 10 06[ 	]+comnlepd %xmm3,%xmm2,%xmm1
[ 	]+2170:[ 	]+0f 25 2d d3 10 07[ 	]+comordpd %xmm3,%xmm2,%xmm1
[ 	]+2176:[ 	]+0f 25 2d d3 10 08[ 	]+comueqpd %xmm3,%xmm2,%xmm1
[ 	]+217c:[ 	]+0f 25 2d d3 10 09[ 	]+comultpd %xmm3,%xmm2,%xmm1
[ 	]+2182:[ 	]+0f 25 2d d3 10 0a[ 	]+comulepd %xmm3,%xmm2,%xmm1
[ 	]+2188:[ 	]+0f 25 2d d3 10 0b[ 	]+comfalsepd %xmm3,%xmm2,%xmm1
[ 	]+218e:[ 	]+0f 25 2d d3 10 0c[ 	]+comunepd %xmm3,%xmm2,%xmm1
[ 	]+2194:[ 	]+0f 25 2d d3 10 0d[ 	]+comunltpd %xmm3,%xmm2,%xmm1
[ 	]+219a:[ 	]+0f 25 2d d3 10 0e[ 	]+comunlepd %xmm3,%xmm2,%xmm1
[ 	]+21a0:[ 	]+0f 25 2d d3 10 0f[ 	]+comtruepd %xmm3,%xmm2,%xmm1
[ 	]+21a6:[ 	]+0f 25 2d 52 10 04 00[ 	]+comeqpd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+21ad:[ 	]+0f 25 2d 52 10 04 01[ 	]+comltpd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+21b4:[ 	]+0f 25 2d 52 10 04 02[ 	]+comlepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+21bb:[ 	]+0f 25 2d 52 10 04 03[ 	]+comunordpd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+21c2:[ 	]+0f 25 2d 52 10 04 04[ 	]+comnepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+21c9:[ 	]+0f 25 2d 52 10 04 05[ 	]+comnltpd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+21d0:[ 	]+0f 25 2d 52 10 04 06[ 	]+comnlepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+21d7:[ 	]+0f 25 2d 52 10 04 07[ 	]+comordpd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+21de:[ 	]+0f 25 2d 52 10 04 08[ 	]+comueqpd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+21e5:[ 	]+0f 25 2d 52 10 04 09[ 	]+comultpd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+21ec:[ 	]+0f 25 2d 52 10 04 0a[ 	]+comulepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+21f3:[ 	]+0f 25 2d 52 10 04 0b[ 	]+comfalsepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+21fa:[ 	]+0f 25 2d 52 10 04 0c[ 	]+comunepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2201:[ 	]+0f 25 2d 52 10 04 0d[ 	]+comunltpd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2208:[ 	]+0f 25 2d 52 10 04 0e[ 	]+comunlepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+220f:[ 	]+0f 25 2d 52 10 04 0f[ 	]+comtruepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2216:[ 	]+0f 25 4c d3 10 04[ 	]+pcomeqb %xmm3,%xmm2,%xmm1
[ 	]+221c:[ 	]+0f 25 4c 52 10 04 04[ 	]+pcomeqb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2223:[ 	]+0f 25 4c d3 10 00[ 	]+pcomltb %xmm3,%xmm2,%xmm1
[ 	]+2229:[ 	]+0f 25 4c d3 10 01[ 	]+pcomleb %xmm3,%xmm2,%xmm1
[ 	]+222f:[ 	]+0f 25 4c d3 10 02[ 	]+pcomgtb %xmm3,%xmm2,%xmm1
[ 	]+2235:[ 	]+0f 25 4c d3 10 03[ 	]+pcomgeb %xmm3,%xmm2,%xmm1
[ 	]+223b:[ 	]+0f 25 4c d3 10 04[ 	]+pcomeqb %xmm3,%xmm2,%xmm1
[ 	]+2241:[ 	]+0f 25 4c d3 10 05[ 	]+pcomneb %xmm3,%xmm2,%xmm1
[ 	]+2247:[ 	]+0f 25 4c 52 10 04 00[ 	]+pcomltb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+224e:[ 	]+0f 25 4c 52 10 04 01[ 	]+pcomleb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2255:[ 	]+0f 25 4c 52 10 04 02[ 	]+pcomgtb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+225c:[ 	]+0f 25 4c 52 10 04 03[ 	]+pcomgeb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2263:[ 	]+0f 25 4c 52 10 04 04[ 	]+pcomeqb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+226a:[ 	]+0f 25 4c 52 10 04 05[ 	]+pcomneb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2271:[ 	]+0f 25 4d d3 10 04[ 	]+pcomeqw %xmm3,%xmm2,%xmm1
[ 	]+2277:[ 	]+0f 25 4d 52 10 04 04[ 	]+pcomeqw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+227e:[ 	]+0f 25 4d d3 10 00[ 	]+pcomltw %xmm3,%xmm2,%xmm1
[ 	]+2284:[ 	]+0f 25 4d d3 10 01[ 	]+pcomlew %xmm3,%xmm2,%xmm1
[ 	]+228a:[ 	]+0f 25 4d d3 10 02[ 	]+pcomgtw %xmm3,%xmm2,%xmm1
[ 	]+2290:[ 	]+0f 25 4d d3 10 03[ 	]+pcomgew %xmm3,%xmm2,%xmm1
[ 	]+2296:[ 	]+0f 25 4d d3 10 04[ 	]+pcomeqw %xmm3,%xmm2,%xmm1
[ 	]+229c:[ 	]+0f 25 4d d3 10 05[ 	]+pcomnew %xmm3,%xmm2,%xmm1
[ 	]+22a2:[ 	]+0f 25 4d 52 10 04 00[ 	]+pcomltw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+22a9:[ 	]+0f 25 4d 52 10 04 01[ 	]+pcomlew 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+22b0:[ 	]+0f 25 4d 52 10 04 02[ 	]+pcomgtw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+22b7:[ 	]+0f 25 4d 52 10 04 03[ 	]+pcomgew 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+22be:[ 	]+0f 25 4d 52 10 04 04[ 	]+pcomeqw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+22c5:[ 	]+0f 25 4d 52 10 04 05[ 	]+pcomnew 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+22cc:[ 	]+0f 25 4e d3 10 04[ 	]+pcomeqd %xmm3,%xmm2,%xmm1
[ 	]+22d2:[ 	]+0f 25 4e 52 10 04 04[ 	]+pcomeqd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+22d9:[ 	]+0f 25 4e d3 10 00[ 	]+pcomltd %xmm3,%xmm2,%xmm1
[ 	]+22df:[ 	]+0f 25 4e d3 10 01[ 	]+pcomled %xmm3,%xmm2,%xmm1
[ 	]+22e5:[ 	]+0f 25 4e d3 10 02[ 	]+pcomgtd %xmm3,%xmm2,%xmm1
[ 	]+22eb:[ 	]+0f 25 4e d3 10 03[ 	]+pcomged %xmm3,%xmm2,%xmm1
[ 	]+22f1:[ 	]+0f 25 4e d3 10 04[ 	]+pcomeqd %xmm3,%xmm2,%xmm1
[ 	]+22f7:[ 	]+0f 25 4e d3 10 05[ 	]+pcomned %xmm3,%xmm2,%xmm1
[ 	]+22fd:[ 	]+0f 25 4e 52 10 04 00[ 	]+pcomltd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2304:[ 	]+0f 25 4e 52 10 04 01[ 	]+pcomled 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+230b:[ 	]+0f 25 4e 52 10 04 02[ 	]+pcomgtd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2312:[ 	]+0f 25 4e 52 10 04 03[ 	]+pcomged 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2319:[ 	]+0f 25 4e 52 10 04 04[ 	]+pcomeqd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2320:[ 	]+0f 25 4e 52 10 04 05[ 	]+pcomned 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2327:[ 	]+0f 25 4f d3 10 04[ 	]+pcomeqq %xmm3,%xmm2,%xmm1
[ 	]+232d:[ 	]+0f 25 4f 52 10 04 04[ 	]+pcomeqq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2334:[ 	]+0f 25 4f d3 10 00[ 	]+pcomltq %xmm3,%xmm2,%xmm1
[ 	]+233a:[ 	]+0f 25 4f d3 10 01[ 	]+pcomleq %xmm3,%xmm2,%xmm1
[ 	]+2340:[ 	]+0f 25 4f d3 10 02[ 	]+pcomgtq %xmm3,%xmm2,%xmm1
[ 	]+2346:[ 	]+0f 25 4f d3 10 03[ 	]+pcomgeq %xmm3,%xmm2,%xmm1
[ 	]+234c:[ 	]+0f 25 4f d3 10 04[ 	]+pcomeqq %xmm3,%xmm2,%xmm1
[ 	]+2352:[ 	]+0f 25 4f d3 10 05[ 	]+pcomneq %xmm3,%xmm2,%xmm1
[ 	]+2358:[ 	]+0f 25 4f 52 10 04 00[ 	]+pcomltq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+235f:[ 	]+0f 25 4f 52 10 04 01[ 	]+pcomleq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2366:[ 	]+0f 25 4f 52 10 04 02[ 	]+pcomgtq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+236d:[ 	]+0f 25 4f 52 10 04 03[ 	]+pcomgeq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2374:[ 	]+0f 25 4f 52 10 04 04[ 	]+pcomeqq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+237b:[ 	]+0f 25 4f 52 10 04 05[ 	]+pcomneq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2382:[ 	]+0f 25 6c d3 10 04[ 	]+pcomequb %xmm3,%xmm2,%xmm1
[ 	]+2388:[ 	]+0f 25 6c 52 10 04 04[ 	]+pcomequb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+238f:[ 	]+0f 25 6c d3 10 00[ 	]+pcomltub %xmm3,%xmm2,%xmm1
[ 	]+2395:[ 	]+0f 25 6c d3 10 01[ 	]+pcomleub %xmm3,%xmm2,%xmm1
[ 	]+239b:[ 	]+0f 25 6c d3 10 02[ 	]+pcomgtub %xmm3,%xmm2,%xmm1
[ 	]+23a1:[ 	]+0f 25 6c d3 10 03[ 	]+pcomgeub %xmm3,%xmm2,%xmm1
[ 	]+23a7:[ 	]+0f 25 6c d3 10 04[ 	]+pcomequb %xmm3,%xmm2,%xmm1
[ 	]+23ad:[ 	]+0f 25 6c d3 10 05[ 	]+pcomneub %xmm3,%xmm2,%xmm1
[ 	]+23b3:[ 	]+0f 25 6c 52 10 04 00[ 	]+pcomltub 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+23ba:[ 	]+0f 25 6c 52 10 04 01[ 	]+pcomleub 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+23c1:[ 	]+0f 25 6c 52 10 04 02[ 	]+pcomgtub 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+23c8:[ 	]+0f 25 6c 52 10 04 03[ 	]+pcomgeub 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+23cf:[ 	]+0f 25 6c 52 10 04 04[ 	]+pcomequb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+23d6:[ 	]+0f 25 6c 52 10 04 05[ 	]+pcomneub 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+23dd:[ 	]+0f 25 6d d3 10 04[ 	]+pcomequw %xmm3,%xmm2,%xmm1
[ 	]+23e3:[ 	]+0f 25 6d 52 10 04 04[ 	]+pcomequw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+23ea:[ 	]+0f 25 6d d3 10 00[ 	]+pcomltuw %xmm3,%xmm2,%xmm1
[ 	]+23f0:[ 	]+0f 25 6d d3 10 01[ 	]+pcomleuw %xmm3,%xmm2,%xmm1
[ 	]+23f6:[ 	]+0f 25 6d d3 10 02[ 	]+pcomgtuw %xmm3,%xmm2,%xmm1
[ 	]+23fc:[ 	]+0f 25 6d d3 10 03[ 	]+pcomgeuw %xmm3,%xmm2,%xmm1
[ 	]+2402:[ 	]+0f 25 6d d3 10 04[ 	]+pcomequw %xmm3,%xmm2,%xmm1
[ 	]+2408:[ 	]+0f 25 6d d3 10 05[ 	]+pcomneuw %xmm3,%xmm2,%xmm1
[ 	]+240e:[ 	]+0f 25 6d 52 10 04 00[ 	]+pcomltuw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2415:[ 	]+0f 25 6d 52 10 04 01[ 	]+pcomleuw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+241c:[ 	]+0f 25 6d 52 10 04 02[ 	]+pcomgtuw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2423:[ 	]+0f 25 6d 52 10 04 03[ 	]+pcomgeuw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+242a:[ 	]+0f 25 6d 52 10 04 04[ 	]+pcomequw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2431:[ 	]+0f 25 6d 52 10 04 05[ 	]+pcomneuw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2438:[ 	]+0f 25 6e d3 10 04[ 	]+pcomequd %xmm3,%xmm2,%xmm1
[ 	]+243e:[ 	]+0f 25 6e 52 10 04 04[ 	]+pcomequd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2445:[ 	]+0f 25 6e d3 10 00[ 	]+pcomltud %xmm3,%xmm2,%xmm1
[ 	]+244b:[ 	]+0f 25 6e d3 10 01[ 	]+pcomleud %xmm3,%xmm2,%xmm1
[ 	]+2451:[ 	]+0f 25 6e d3 10 02[ 	]+pcomgtud %xmm3,%xmm2,%xmm1
[ 	]+2457:[ 	]+0f 25 6e d3 10 03[ 	]+pcomgeud %xmm3,%xmm2,%xmm1
[ 	]+245d:[ 	]+0f 25 6e d3 10 04[ 	]+pcomequd %xmm3,%xmm2,%xmm1
[ 	]+2463:[ 	]+0f 25 6e d3 10 05[ 	]+pcomneud %xmm3,%xmm2,%xmm1
[ 	]+2469:[ 	]+0f 25 6e 52 10 04 00[ 	]+pcomltud 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2470:[ 	]+0f 25 6e 52 10 04 01[ 	]+pcomleud 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2477:[ 	]+0f 25 6e 52 10 04 02[ 	]+pcomgtud 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+247e:[ 	]+0f 25 6e 52 10 04 03[ 	]+pcomgeud 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2485:[ 	]+0f 25 6e 52 10 04 04[ 	]+pcomequd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+248c:[ 	]+0f 25 6e 52 10 04 05[ 	]+pcomneud 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+2493:[ 	]+0f 25 6f d3 10 04[ 	]+pcomequq %xmm3,%xmm2,%xmm1
[ 	]+2499:[ 	]+0f 25 6f 52 10 04 04[ 	]+pcomequq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+24a0:[ 	]+0f 25 6f d3 10 00[ 	]+pcomltuq %xmm3,%xmm2,%xmm1
[ 	]+24a6:[ 	]+0f 25 6f d3 10 01[ 	]+pcomleuq %xmm3,%xmm2,%xmm1
[ 	]+24ac:[ 	]+0f 25 6f d3 10 02[ 	]+pcomgtuq %xmm3,%xmm2,%xmm1
[ 	]+24b2:[ 	]+0f 25 6f d3 10 03[ 	]+pcomgeuq %xmm3,%xmm2,%xmm1
[ 	]+24b8:[ 	]+0f 25 6f d3 10 04[ 	]+pcomequq %xmm3,%xmm2,%xmm1
[ 	]+24be:[ 	]+0f 25 6f d3 10 05[ 	]+pcomneuq %xmm3,%xmm2,%xmm1
[ 	]+24c4:[ 	]+0f 25 6f 52 10 04 00[ 	]+pcomltuq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+24cb:[ 	]+0f 25 6f 52 10 04 01[ 	]+pcomleuq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+24d2:[ 	]+0f 25 6f 52 10 04 02[ 	]+pcomgtuq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+24d9:[ 	]+0f 25 6f 52 10 04 03[ 	]+pcomgeuq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+24e0:[ 	]+0f 25 6f 52 10 04 04[ 	]+pcomequq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+24e7:[ 	]+0f 25 6f 52 10 04 05[ 	]+pcomneuq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+24ee:[ 	]+0f 25 2e e5 b5 04[ 	]+comness %xmm13,%xmm12,%xmm11
[ 	]+24f4:[ 	]+0f 25 2e a7 b5 00 00 10 00 04[ 	]+comness 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+24fe:[ 	]+0f 25 2e e5 b5 00[ 	]+comeqss %xmm13,%xmm12,%xmm11
[ 	]+2504:[ 	]+0f 25 2e e5 b5 01[ 	]+comltss %xmm13,%xmm12,%xmm11
[ 	]+250a:[ 	]+0f 25 2e e5 b5 02[ 	]+comless %xmm13,%xmm12,%xmm11
[ 	]+2510:[ 	]+0f 25 2e e5 b5 03[ 	]+comunordss %xmm13,%xmm12,%xmm11
[ 	]+2516:[ 	]+0f 25 2e e5 b5 04[ 	]+comness %xmm13,%xmm12,%xmm11
[ 	]+251c:[ 	]+0f 25 2e e5 b5 05[ 	]+comnltss %xmm13,%xmm12,%xmm11
[ 	]+2522:[ 	]+0f 25 2e e5 b5 06[ 	]+comnless %xmm13,%xmm12,%xmm11
[ 	]+2528:[ 	]+0f 25 2e e5 b5 07[ 	]+comordss %xmm13,%xmm12,%xmm11
[ 	]+252e:[ 	]+0f 25 2e e5 b5 08[ 	]+comueqss %xmm13,%xmm12,%xmm11
[ 	]+2534:[ 	]+0f 25 2e e5 b5 09[ 	]+comultss %xmm13,%xmm12,%xmm11
[ 	]+253a:[ 	]+0f 25 2e e5 b5 0a[ 	]+comuless %xmm13,%xmm12,%xmm11
[ 	]+2540:[ 	]+0f 25 2e e5 b5 0b[ 	]+comfalsess %xmm13,%xmm12,%xmm11
[ 	]+2546:[ 	]+0f 25 2e e5 b5 0c[ 	]+comuness %xmm13,%xmm12,%xmm11
[ 	]+254c:[ 	]+0f 25 2e e5 b5 0d[ 	]+comunltss %xmm13,%xmm12,%xmm11
[ 	]+2552:[ 	]+0f 25 2e e5 b5 0e[ 	]+comunless %xmm13,%xmm12,%xmm11
[ 	]+2558:[ 	]+0f 25 2e e5 b5 0f[ 	]+comtruess %xmm13,%xmm12,%xmm11
[ 	]+255e:[ 	]+0f 25 2e a7 b5 00 00 10 00 00[ 	]+comeqss 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2568:[ 	]+0f 25 2e a7 b5 00 00 10 00 01[ 	]+comltss 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2572:[ 	]+0f 25 2e a7 b5 00 00 10 00 02[ 	]+comless 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+257c:[ 	]+0f 25 2e a7 b5 00 00 10 00 03[ 	]+comunordss 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2586:[ 	]+0f 25 2e a7 b5 00 00 10 00 04[ 	]+comness 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2590:[ 	]+0f 25 2e a7 b5 00 00 10 00 05[ 	]+comnltss 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+259a:[ 	]+0f 25 2e a7 b5 00 00 10 00 06[ 	]+comnless 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+25a4:[ 	]+0f 25 2e a7 b5 00 00 10 00 07[ 	]+comordss 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+25ae:[ 	]+0f 25 2e a7 b5 00 00 10 00 08[ 	]+comueqss 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+25b8:[ 	]+0f 25 2e a7 b5 00 00 10 00 09[ 	]+comultss 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+25c2:[ 	]+0f 25 2e a7 b5 00 00 10 00 0a[ 	]+comuless 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+25cc:[ 	]+0f 25 2e a7 b5 00 00 10 00 0b[ 	]+comfalsess 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+25d6:[ 	]+0f 25 2e a7 b5 00 00 10 00 0c[ 	]+comuness 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+25e0:[ 	]+0f 25 2e a7 b5 00 00 10 00 0d[ 	]+comunltss 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+25ea:[ 	]+0f 25 2e a7 b5 00 00 10 00 0e[ 	]+comunless 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+25f4:[ 	]+0f 25 2e a7 b5 00 00 10 00 0f[ 	]+comtruess 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+25fe:[ 	]+0f 25 2f e5 b5 04[ 	]+comnesd %xmm13,%xmm12,%xmm11
[ 	]+2604:[ 	]+0f 25 2f a7 b5 00 00 10 00 04[ 	]+comnesd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+260e:[ 	]+0f 25 2f e5 b5 00[ 	]+comeqsd %xmm13,%xmm12,%xmm11
[ 	]+2614:[ 	]+0f 25 2f e5 b5 01[ 	]+comltsd %xmm13,%xmm12,%xmm11
[ 	]+261a:[ 	]+0f 25 2f e5 b5 02[ 	]+comlesd %xmm13,%xmm12,%xmm11
[ 	]+2620:[ 	]+0f 25 2f e5 b5 03[ 	]+comunordsd %xmm13,%xmm12,%xmm11
[ 	]+2626:[ 	]+0f 25 2f e5 b5 04[ 	]+comnesd %xmm13,%xmm12,%xmm11
[ 	]+262c:[ 	]+0f 25 2f e5 b5 05[ 	]+comnltsd %xmm13,%xmm12,%xmm11
[ 	]+2632:[ 	]+0f 25 2f e5 b5 06[ 	]+comnlesd %xmm13,%xmm12,%xmm11
[ 	]+2638:[ 	]+0f 25 2f e5 b5 07[ 	]+comordsd %xmm13,%xmm12,%xmm11
[ 	]+263e:[ 	]+0f 25 2f e5 b5 08[ 	]+comueqsd %xmm13,%xmm12,%xmm11
[ 	]+2644:[ 	]+0f 25 2f e5 b5 09[ 	]+comultsd %xmm13,%xmm12,%xmm11
[ 	]+264a:[ 	]+0f 25 2f e5 b5 0a[ 	]+comulesd %xmm13,%xmm12,%xmm11
[ 	]+2650:[ 	]+0f 25 2f e5 b5 0b[ 	]+comfalsesd %xmm13,%xmm12,%xmm11
[ 	]+2656:[ 	]+0f 25 2f e5 b5 0c[ 	]+comunesd %xmm13,%xmm12,%xmm11
[ 	]+265c:[ 	]+0f 25 2f e5 b5 0d[ 	]+comunltsd %xmm13,%xmm12,%xmm11
[ 	]+2662:[ 	]+0f 25 2f e5 b5 0e[ 	]+comunlesd %xmm13,%xmm12,%xmm11
[ 	]+2668:[ 	]+0f 25 2f e5 b5 0f[ 	]+comtruesd %xmm13,%xmm12,%xmm11
[ 	]+266e:[ 	]+0f 25 2f a7 b5 00 00 10 00 00[ 	]+comeqsd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2678:[ 	]+0f 25 2f a7 b5 00 00 10 00 01[ 	]+comltsd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2682:[ 	]+0f 25 2f a7 b5 00 00 10 00 02[ 	]+comlesd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+268c:[ 	]+0f 25 2f a7 b5 00 00 10 00 03[ 	]+comunordsd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2696:[ 	]+0f 25 2f a7 b5 00 00 10 00 04[ 	]+comnesd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+26a0:[ 	]+0f 25 2f a7 b5 00 00 10 00 05[ 	]+comnltsd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+26aa:[ 	]+0f 25 2f a7 b5 00 00 10 00 06[ 	]+comnlesd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+26b4:[ 	]+0f 25 2f a7 b5 00 00 10 00 07[ 	]+comordsd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+26be:[ 	]+0f 25 2f a7 b5 00 00 10 00 08[ 	]+comueqsd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+26c8:[ 	]+0f 25 2f a7 b5 00 00 10 00 09[ 	]+comultsd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+26d2:[ 	]+0f 25 2f a7 b5 00 00 10 00 0a[ 	]+comulesd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+26dc:[ 	]+0f 25 2f a7 b5 00 00 10 00 0b[ 	]+comfalsesd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+26e6:[ 	]+0f 25 2f a7 b5 00 00 10 00 0c[ 	]+comunesd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+26f0:[ 	]+0f 25 2f a7 b5 00 00 10 00 0d[ 	]+comunltsd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+26fa:[ 	]+0f 25 2f a7 b5 00 00 10 00 0e[ 	]+comunlesd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2704:[ 	]+0f 25 2f a7 b5 00 00 10 00 0f[ 	]+comtruesd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+270e:[ 	]+0f 25 2c e5 b5 04[ 	]+comneps %xmm13,%xmm12,%xmm11
[ 	]+2714:[ 	]+0f 25 2c a7 b5 00 00 10 00 04[ 	]+comneps 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+271e:[ 	]+0f 25 2c e5 b5 00[ 	]+comeqps %xmm13,%xmm12,%xmm11
[ 	]+2724:[ 	]+0f 25 2c e5 b5 01[ 	]+comltps %xmm13,%xmm12,%xmm11
[ 	]+272a:[ 	]+0f 25 2c e5 b5 02[ 	]+comleps %xmm13,%xmm12,%xmm11
[ 	]+2730:[ 	]+0f 25 2c e5 b5 03[ 	]+comunordps %xmm13,%xmm12,%xmm11
[ 	]+2736:[ 	]+0f 25 2c e5 b5 04[ 	]+comneps %xmm13,%xmm12,%xmm11
[ 	]+273c:[ 	]+0f 25 2c e5 b5 05[ 	]+comnltps %xmm13,%xmm12,%xmm11
[ 	]+2742:[ 	]+0f 25 2c e5 b5 06[ 	]+comnleps %xmm13,%xmm12,%xmm11
[ 	]+2748:[ 	]+0f 25 2c e5 b5 07[ 	]+comordps %xmm13,%xmm12,%xmm11
[ 	]+274e:[ 	]+0f 25 2c e5 b5 08[ 	]+comueqps %xmm13,%xmm12,%xmm11
[ 	]+2754:[ 	]+0f 25 2c e5 b5 09[ 	]+comultps %xmm13,%xmm12,%xmm11
[ 	]+275a:[ 	]+0f 25 2c e5 b5 0a[ 	]+comuleps %xmm13,%xmm12,%xmm11
[ 	]+2760:[ 	]+0f 25 2c e5 b5 0b[ 	]+comfalseps %xmm13,%xmm12,%xmm11
[ 	]+2766:[ 	]+0f 25 2c e5 b5 0c[ 	]+comuneps %xmm13,%xmm12,%xmm11
[ 	]+276c:[ 	]+0f 25 2c e5 b5 0d[ 	]+comunltps %xmm13,%xmm12,%xmm11
[ 	]+2772:[ 	]+0f 25 2c e5 b5 0e[ 	]+comunleps %xmm13,%xmm12,%xmm11
[ 	]+2778:[ 	]+0f 25 2c e5 b5 0f[ 	]+comtrueps %xmm13,%xmm12,%xmm11
[ 	]+277e:[ 	]+0f 25 2c a7 b5 00 00 10 00 00[ 	]+comeqps 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2788:[ 	]+0f 25 2c a7 b5 00 00 10 00 01[ 	]+comltps 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2792:[ 	]+0f 25 2c a7 b5 00 00 10 00 02[ 	]+comleps 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+279c:[ 	]+0f 25 2c a7 b5 00 00 10 00 03[ 	]+comunordps 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+27a6:[ 	]+0f 25 2c a7 b5 00 00 10 00 04[ 	]+comneps 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+27b0:[ 	]+0f 25 2c a7 b5 00 00 10 00 05[ 	]+comnltps 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+27ba:[ 	]+0f 25 2c a7 b5 00 00 10 00 06[ 	]+comnleps 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+27c4:[ 	]+0f 25 2c a7 b5 00 00 10 00 07[ 	]+comordps 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+27ce:[ 	]+0f 25 2c a7 b5 00 00 10 00 08[ 	]+comueqps 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+27d8:[ 	]+0f 25 2c a7 b5 00 00 10 00 09[ 	]+comultps 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+27e2:[ 	]+0f 25 2c a7 b5 00 00 10 00 0a[ 	]+comuleps 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+27ec:[ 	]+0f 25 2c a7 b5 00 00 10 00 0b[ 	]+comfalseps 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+27f6:[ 	]+0f 25 2c a7 b5 00 00 10 00 0c[ 	]+comuneps 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2800:[ 	]+0f 25 2c a7 b5 00 00 10 00 0d[ 	]+comunltps 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+280a:[ 	]+0f 25 2c a7 b5 00 00 10 00 0e[ 	]+comunleps 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2814:[ 	]+0f 25 2c a7 b5 00 00 10 00 0f[ 	]+comtrueps 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+281e:[ 	]+0f 25 2d e5 b5 04[ 	]+comnepd %xmm13,%xmm12,%xmm11
[ 	]+2824:[ 	]+0f 25 2d a7 b5 00 00 10 00 04[ 	]+comnepd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+282e:[ 	]+0f 25 2d e5 b5 00[ 	]+comeqpd %xmm13,%xmm12,%xmm11
[ 	]+2834:[ 	]+0f 25 2d e5 b5 01[ 	]+comltpd %xmm13,%xmm12,%xmm11
[ 	]+283a:[ 	]+0f 25 2d e5 b5 02[ 	]+comlepd %xmm13,%xmm12,%xmm11
[ 	]+2840:[ 	]+0f 25 2d e5 b5 03[ 	]+comunordpd %xmm13,%xmm12,%xmm11
[ 	]+2846:[ 	]+0f 25 2d e5 b5 04[ 	]+comnepd %xmm13,%xmm12,%xmm11
[ 	]+284c:[ 	]+0f 25 2d e5 b5 05[ 	]+comnltpd %xmm13,%xmm12,%xmm11
[ 	]+2852:[ 	]+0f 25 2d e5 b5 06[ 	]+comnlepd %xmm13,%xmm12,%xmm11
[ 	]+2858:[ 	]+0f 25 2d e5 b5 07[ 	]+comordpd %xmm13,%xmm12,%xmm11
[ 	]+285e:[ 	]+0f 25 2d e5 b5 08[ 	]+comueqpd %xmm13,%xmm12,%xmm11
[ 	]+2864:[ 	]+0f 25 2d e5 b5 09[ 	]+comultpd %xmm13,%xmm12,%xmm11
[ 	]+286a:[ 	]+0f 25 2d e5 b5 0a[ 	]+comulepd %xmm13,%xmm12,%xmm11
[ 	]+2870:[ 	]+0f 25 2d e5 b5 0b[ 	]+comfalsepd %xmm13,%xmm12,%xmm11
[ 	]+2876:[ 	]+0f 25 2d e5 b5 0c[ 	]+comunepd %xmm13,%xmm12,%xmm11
[ 	]+287c:[ 	]+0f 25 2d e5 b5 0d[ 	]+comunltpd %xmm13,%xmm12,%xmm11
[ 	]+2882:[ 	]+0f 25 2d e5 b5 0e[ 	]+comunlepd %xmm13,%xmm12,%xmm11
[ 	]+2888:[ 	]+0f 25 2d e5 b5 0f[ 	]+comtruepd %xmm13,%xmm12,%xmm11
[ 	]+288e:[ 	]+0f 25 2d a7 b5 00 00 10 00 00[ 	]+comeqpd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2898:[ 	]+0f 25 2d a7 b5 00 00 10 00 01[ 	]+comltpd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+28a2:[ 	]+0f 25 2d a7 b5 00 00 10 00 02[ 	]+comlepd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+28ac:[ 	]+0f 25 2d a7 b5 00 00 10 00 03[ 	]+comunordpd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+28b6:[ 	]+0f 25 2d a7 b5 00 00 10 00 04[ 	]+comnepd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+28c0:[ 	]+0f 25 2d a7 b5 00 00 10 00 05[ 	]+comnltpd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+28ca:[ 	]+0f 25 2d a7 b5 00 00 10 00 06[ 	]+comnlepd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+28d4:[ 	]+0f 25 2d a7 b5 00 00 10 00 07[ 	]+comordpd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+28de:[ 	]+0f 25 2d a7 b5 00 00 10 00 08[ 	]+comueqpd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+28e8:[ 	]+0f 25 2d a7 b5 00 00 10 00 09[ 	]+comultpd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+28f2:[ 	]+0f 25 2d a7 b5 00 00 10 00 0a[ 	]+comulepd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+28fc:[ 	]+0f 25 2d a7 b5 00 00 10 00 0b[ 	]+comfalsepd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2906:[ 	]+0f 25 2d a7 b5 00 00 10 00 0c[ 	]+comunepd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2910:[ 	]+0f 25 2d a7 b5 00 00 10 00 0d[ 	]+comunltpd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+291a:[ 	]+0f 25 2d a7 b5 00 00 10 00 0e[ 	]+comunlepd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2924:[ 	]+0f 25 2d a7 b5 00 00 10 00 0f[ 	]+comtruepd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+292e:[ 	]+0f 25 4c e5 b5 04[ 	]+pcomeqb %xmm13,%xmm12,%xmm11
[ 	]+2934:[ 	]+0f 25 4c a7 b5 00 00 10 00 04[ 	]+pcomeqb 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+293e:[ 	]+0f 25 4c e5 b5 00[ 	]+pcomltb %xmm13,%xmm12,%xmm11
[ 	]+2944:[ 	]+0f 25 4c e5 b5 01[ 	]+pcomleb %xmm13,%xmm12,%xmm11
[ 	]+294a:[ 	]+0f 25 4c e5 b5 02[ 	]+pcomgtb %xmm13,%xmm12,%xmm11
[ 	]+2950:[ 	]+0f 25 4c e5 b5 03[ 	]+pcomgeb %xmm13,%xmm12,%xmm11
[ 	]+2956:[ 	]+0f 25 4c e5 b5 04[ 	]+pcomeqb %xmm13,%xmm12,%xmm11
[ 	]+295c:[ 	]+0f 25 4c e5 b5 05[ 	]+pcomneb %xmm13,%xmm12,%xmm11
[ 	]+2962:[ 	]+0f 25 4c a7 b5 00 00 10 00 00[ 	]+pcomltb 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+296c:[ 	]+0f 25 4c a7 b5 00 00 10 00 01[ 	]+pcomleb 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2976:[ 	]+0f 25 4c a7 b5 00 00 10 00 02[ 	]+pcomgtb 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2980:[ 	]+0f 25 4c a7 b5 00 00 10 00 03[ 	]+pcomgeb 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+298a:[ 	]+0f 25 4c a7 b5 00 00 10 00 04[ 	]+pcomeqb 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2994:[ 	]+0f 25 4c a7 b5 00 00 10 00 05[ 	]+pcomneb 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+299e:[ 	]+0f 25 4d e5 b5 04[ 	]+pcomeqw %xmm13,%xmm12,%xmm11
[ 	]+29a4:[ 	]+0f 25 4d a7 b5 00 00 10 00 04[ 	]+pcomeqw 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+29ae:[ 	]+0f 25 4d e5 b5 00[ 	]+pcomltw %xmm13,%xmm12,%xmm11
[ 	]+29b4:[ 	]+0f 25 4d e5 b5 01[ 	]+pcomlew %xmm13,%xmm12,%xmm11
[ 	]+29ba:[ 	]+0f 25 4d e5 b5 02[ 	]+pcomgtw %xmm13,%xmm12,%xmm11
[ 	]+29c0:[ 	]+0f 25 4d e5 b5 03[ 	]+pcomgew %xmm13,%xmm12,%xmm11
[ 	]+29c6:[ 	]+0f 25 4d e5 b5 04[ 	]+pcomeqw %xmm13,%xmm12,%xmm11
[ 	]+29cc:[ 	]+0f 25 4d e5 b5 05[ 	]+pcomnew %xmm13,%xmm12,%xmm11
[ 	]+29d2:[ 	]+0f 25 4d a7 b5 00 00 10 00 00[ 	]+pcomltw 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+29dc:[ 	]+0f 25 4d a7 b5 00 00 10 00 01[ 	]+pcomlew 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+29e6:[ 	]+0f 25 4d a7 b5 00 00 10 00 02[ 	]+pcomgtw 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+29f0:[ 	]+0f 25 4d a7 b5 00 00 10 00 03[ 	]+pcomgew 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+29fa:[ 	]+0f 25 4d a7 b5 00 00 10 00 04[ 	]+pcomeqw 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2a04:[ 	]+0f 25 4d a7 b5 00 00 10 00 05[ 	]+pcomnew 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2a0e:[ 	]+0f 25 4e e5 b5 04[ 	]+pcomeqd %xmm13,%xmm12,%xmm11
[ 	]+2a14:[ 	]+0f 25 4e a7 b5 00 00 10 00 04[ 	]+pcomeqd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2a1e:[ 	]+0f 25 4e e5 b5 00[ 	]+pcomltd %xmm13,%xmm12,%xmm11
[ 	]+2a24:[ 	]+0f 25 4e e5 b5 01[ 	]+pcomled %xmm13,%xmm12,%xmm11
[ 	]+2a2a:[ 	]+0f 25 4e e5 b5 02[ 	]+pcomgtd %xmm13,%xmm12,%xmm11
[ 	]+2a30:[ 	]+0f 25 4e e5 b5 03[ 	]+pcomged %xmm13,%xmm12,%xmm11
[ 	]+2a36:[ 	]+0f 25 4e e5 b5 04[ 	]+pcomeqd %xmm13,%xmm12,%xmm11
[ 	]+2a3c:[ 	]+0f 25 4e e5 b5 05[ 	]+pcomned %xmm13,%xmm12,%xmm11
[ 	]+2a42:[ 	]+0f 25 4e a7 b5 00 00 10 00 00[ 	]+pcomltd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2a4c:[ 	]+0f 25 4e a7 b5 00 00 10 00 01[ 	]+pcomled 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2a56:[ 	]+0f 25 4e a7 b5 00 00 10 00 02[ 	]+pcomgtd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2a60:[ 	]+0f 25 4e a7 b5 00 00 10 00 03[ 	]+pcomged 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2a6a:[ 	]+0f 25 4e a7 b5 00 00 10 00 04[ 	]+pcomeqd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2a74:[ 	]+0f 25 4e a7 b5 00 00 10 00 05[ 	]+pcomned 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2a7e:[ 	]+0f 25 4f e5 b5 04[ 	]+pcomeqq %xmm13,%xmm12,%xmm11
[ 	]+2a84:[ 	]+0f 25 4f a7 b5 00 00 10 00 04[ 	]+pcomeqq 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2a8e:[ 	]+0f 25 4f e5 b5 00[ 	]+pcomltq %xmm13,%xmm12,%xmm11
[ 	]+2a94:[ 	]+0f 25 4f e5 b5 01[ 	]+pcomleq %xmm13,%xmm12,%xmm11
[ 	]+2a9a:[ 	]+0f 25 4f e5 b5 02[ 	]+pcomgtq %xmm13,%xmm12,%xmm11
[ 	]+2aa0:[ 	]+0f 25 4f e5 b5 03[ 	]+pcomgeq %xmm13,%xmm12,%xmm11
[ 	]+2aa6:[ 	]+0f 25 4f e5 b5 04[ 	]+pcomeqq %xmm13,%xmm12,%xmm11
[ 	]+2aac:[ 	]+0f 25 4f e5 b5 05[ 	]+pcomneq %xmm13,%xmm12,%xmm11
[ 	]+2ab2:[ 	]+0f 25 4f a7 b5 00 00 10 00 00[ 	]+pcomltq 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2abc:[ 	]+0f 25 4f a7 b5 00 00 10 00 01[ 	]+pcomleq 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2ac6:[ 	]+0f 25 4f a7 b5 00 00 10 00 02[ 	]+pcomgtq 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2ad0:[ 	]+0f 25 4f a7 b5 00 00 10 00 03[ 	]+pcomgeq 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2ada:[ 	]+0f 25 4f a7 b5 00 00 10 00 04[ 	]+pcomeqq 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2ae4:[ 	]+0f 25 4f a7 b5 00 00 10 00 05[ 	]+pcomneq 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2aee:[ 	]+0f 25 6c e5 b5 04[ 	]+pcomequb %xmm13,%xmm12,%xmm11
[ 	]+2af4:[ 	]+0f 25 6c a7 b5 00 00 10 00 04[ 	]+pcomequb 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2afe:[ 	]+0f 25 6c e5 b5 00[ 	]+pcomltub %xmm13,%xmm12,%xmm11
[ 	]+2b04:[ 	]+0f 25 6c e5 b5 01[ 	]+pcomleub %xmm13,%xmm12,%xmm11
[ 	]+2b0a:[ 	]+0f 25 6c e5 b5 02[ 	]+pcomgtub %xmm13,%xmm12,%xmm11
[ 	]+2b10:[ 	]+0f 25 6c e5 b5 03[ 	]+pcomgeub %xmm13,%xmm12,%xmm11
[ 	]+2b16:[ 	]+0f 25 6c e5 b5 04[ 	]+pcomequb %xmm13,%xmm12,%xmm11
[ 	]+2b1c:[ 	]+0f 25 6c e5 b5 05[ 	]+pcomneub %xmm13,%xmm12,%xmm11
[ 	]+2b22:[ 	]+0f 25 6c a7 b5 00 00 10 00 00[ 	]+pcomltub 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2b2c:[ 	]+0f 25 6c a7 b5 00 00 10 00 01[ 	]+pcomleub 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2b36:[ 	]+0f 25 6c a7 b5 00 00 10 00 02[ 	]+pcomgtub 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2b40:[ 	]+0f 25 6c a7 b5 00 00 10 00 03[ 	]+pcomgeub 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2b4a:[ 	]+0f 25 6c a7 b5 00 00 10 00 04[ 	]+pcomequb 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2b54:[ 	]+0f 25 6c a7 b5 00 00 10 00 05[ 	]+pcomneub 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2b5e:[ 	]+0f 25 6d e5 b5 04[ 	]+pcomequw %xmm13,%xmm12,%xmm11
[ 	]+2b64:[ 	]+0f 25 6d a7 b5 00 00 10 00 04[ 	]+pcomequw 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2b6e:[ 	]+0f 25 6d e5 b5 00[ 	]+pcomltuw %xmm13,%xmm12,%xmm11
[ 	]+2b74:[ 	]+0f 25 6d e5 b5 01[ 	]+pcomleuw %xmm13,%xmm12,%xmm11
[ 	]+2b7a:[ 	]+0f 25 6d e5 b5 02[ 	]+pcomgtuw %xmm13,%xmm12,%xmm11
[ 	]+2b80:[ 	]+0f 25 6d e5 b5 03[ 	]+pcomgeuw %xmm13,%xmm12,%xmm11
[ 	]+2b86:[ 	]+0f 25 6d e5 b5 04[ 	]+pcomequw %xmm13,%xmm12,%xmm11
[ 	]+2b8c:[ 	]+0f 25 6d e5 b5 05[ 	]+pcomneuw %xmm13,%xmm12,%xmm11
[ 	]+2b92:[ 	]+0f 25 6d a7 b5 00 00 10 00 00[ 	]+pcomltuw 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2b9c:[ 	]+0f 25 6d a7 b5 00 00 10 00 01[ 	]+pcomleuw 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2ba6:[ 	]+0f 25 6d a7 b5 00 00 10 00 02[ 	]+pcomgtuw 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2bb0:[ 	]+0f 25 6d a7 b5 00 00 10 00 03[ 	]+pcomgeuw 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2bba:[ 	]+0f 25 6d a7 b5 00 00 10 00 04[ 	]+pcomequw 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2bc4:[ 	]+0f 25 6d a7 b5 00 00 10 00 05[ 	]+pcomneuw 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2bce:[ 	]+0f 25 6e e5 b5 04[ 	]+pcomequd %xmm13,%xmm12,%xmm11
[ 	]+2bd4:[ 	]+0f 25 6e a7 b5 00 00 10 00 04[ 	]+pcomequd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2bde:[ 	]+0f 25 6e e5 b5 00[ 	]+pcomltud %xmm13,%xmm12,%xmm11
[ 	]+2be4:[ 	]+0f 25 6e e5 b5 01[ 	]+pcomleud %xmm13,%xmm12,%xmm11
[ 	]+2bea:[ 	]+0f 25 6e e5 b5 02[ 	]+pcomgtud %xmm13,%xmm12,%xmm11
[ 	]+2bf0:[ 	]+0f 25 6e e5 b5 03[ 	]+pcomgeud %xmm13,%xmm12,%xmm11
[ 	]+2bf6:[ 	]+0f 25 6e e5 b5 04[ 	]+pcomequd %xmm13,%xmm12,%xmm11
[ 	]+2bfc:[ 	]+0f 25 6e e5 b5 05[ 	]+pcomneud %xmm13,%xmm12,%xmm11
[ 	]+2c02:[ 	]+0f 25 6e a7 b5 00 00 10 00 00[ 	]+pcomltud 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2c0c:[ 	]+0f 25 6e a7 b5 00 00 10 00 01[ 	]+pcomleud 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2c16:[ 	]+0f 25 6e a7 b5 00 00 10 00 02[ 	]+pcomgtud 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2c20:[ 	]+0f 25 6e a7 b5 00 00 10 00 03[ 	]+pcomgeud 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2c2a:[ 	]+0f 25 6e a7 b5 00 00 10 00 04[ 	]+pcomequd 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2c34:[ 	]+0f 25 6e a7 b5 00 00 10 00 05[ 	]+pcomneud 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2c3e:[ 	]+0f 25 6f e5 b5 04[ 	]+pcomequq %xmm13,%xmm12,%xmm11
[ 	]+2c44:[ 	]+0f 25 6f a7 b5 00 00 10 00 04[ 	]+pcomequq 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2c4e:[ 	]+0f 25 6f e5 b5 00[ 	]+pcomltuq %xmm13,%xmm12,%xmm11
[ 	]+2c54:[ 	]+0f 25 6f e5 b5 01[ 	]+pcomleuq %xmm13,%xmm12,%xmm11
[ 	]+2c5a:[ 	]+0f 25 6f e5 b5 02[ 	]+pcomgtuq %xmm13,%xmm12,%xmm11
[ 	]+2c60:[ 	]+0f 25 6f e5 b5 03[ 	]+pcomgeuq %xmm13,%xmm12,%xmm11
[ 	]+2c66:[ 	]+0f 25 6f e5 b5 04[ 	]+pcomequq %xmm13,%xmm12,%xmm11
[ 	]+2c6c:[ 	]+0f 25 6f e5 b5 05[ 	]+pcomneuq %xmm13,%xmm12,%xmm11
[ 	]+2c72:[ 	]+0f 25 6f a7 b5 00 00 10 00 00[ 	]+pcomltuq 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2c7c:[ 	]+0f 25 6f a7 b5 00 00 10 00 01[ 	]+pcomleuq 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2c86:[ 	]+0f 25 6f a7 b5 00 00 10 00 02[ 	]+pcomgtuq 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2c90:[ 	]+0f 25 6f a7 b5 00 00 10 00 03[ 	]+pcomgeuq 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2c9a:[ 	]+0f 25 6f a7 b5 00 00 10 00 04[ 	]+pcomequq 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2ca4:[ 	]+0f 25 6f a7 b5 00 00 10 00 05[ 	]+pcomneuq 0x100000\(%r15\),%xmm12,%xmm11
[ 	]+2cae:[ 	]+0f 25 2e e3 14 04[ 	]+comness %xmm3,%xmm12,%xmm1
[ 	]+2cb4:[ 	]+0f 25 2e 62 14 04 04[ 	]+comness 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2cbb:[ 	]+0f 25 2e e3 14 00[ 	]+comeqss %xmm3,%xmm12,%xmm1
[ 	]+2cc1:[ 	]+0f 25 2e e3 14 01[ 	]+comltss %xmm3,%xmm12,%xmm1
[ 	]+2cc7:[ 	]+0f 25 2e e3 14 02[ 	]+comless %xmm3,%xmm12,%xmm1
[ 	]+2ccd:[ 	]+0f 25 2e e3 14 03[ 	]+comunordss %xmm3,%xmm12,%xmm1
[ 	]+2cd3:[ 	]+0f 25 2e e3 14 04[ 	]+comness %xmm3,%xmm12,%xmm1
[ 	]+2cd9:[ 	]+0f 25 2e e3 14 05[ 	]+comnltss %xmm3,%xmm12,%xmm1
[ 	]+2cdf:[ 	]+0f 25 2e e3 14 06[ 	]+comnless %xmm3,%xmm12,%xmm1
[ 	]+2ce5:[ 	]+0f 25 2e e3 14 07[ 	]+comordss %xmm3,%xmm12,%xmm1
[ 	]+2ceb:[ 	]+0f 25 2e e3 14 08[ 	]+comueqss %xmm3,%xmm12,%xmm1
[ 	]+2cf1:[ 	]+0f 25 2e e3 14 09[ 	]+comultss %xmm3,%xmm12,%xmm1
[ 	]+2cf7:[ 	]+0f 25 2e e3 14 0a[ 	]+comuless %xmm3,%xmm12,%xmm1
[ 	]+2cfd:[ 	]+0f 25 2e e3 14 0b[ 	]+comfalsess %xmm3,%xmm12,%xmm1
[ 	]+2d03:[ 	]+0f 25 2e e3 14 0c[ 	]+comuness %xmm3,%xmm12,%xmm1
[ 	]+2d09:[ 	]+0f 25 2e e3 14 0d[ 	]+comunltss %xmm3,%xmm12,%xmm1
[ 	]+2d0f:[ 	]+0f 25 2e e3 14 0e[ 	]+comunless %xmm3,%xmm12,%xmm1
[ 	]+2d15:[ 	]+0f 25 2e e3 14 0f[ 	]+comtruess %xmm3,%xmm12,%xmm1
[ 	]+2d1b:[ 	]+0f 25 2e 62 14 04 00[ 	]+comeqss 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2d22:[ 	]+0f 25 2e 62 14 04 01[ 	]+comltss 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2d29:[ 	]+0f 25 2e 62 14 04 02[ 	]+comless 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2d30:[ 	]+0f 25 2e 62 14 04 03[ 	]+comunordss 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2d37:[ 	]+0f 25 2e 62 14 04 04[ 	]+comness 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2d3e:[ 	]+0f 25 2e 62 14 04 05[ 	]+comnltss 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2d45:[ 	]+0f 25 2e 62 14 04 06[ 	]+comnless 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2d4c:[ 	]+0f 25 2e 62 14 04 07[ 	]+comordss 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2d53:[ 	]+0f 25 2e 62 14 04 08[ 	]+comueqss 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2d5a:[ 	]+0f 25 2e 62 14 04 09[ 	]+comultss 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2d61:[ 	]+0f 25 2e 62 14 04 0a[ 	]+comuless 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2d68:[ 	]+0f 25 2e 62 14 04 0b[ 	]+comfalsess 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2d6f:[ 	]+0f 25 2e 62 14 04 0c[ 	]+comuness 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2d76:[ 	]+0f 25 2e 62 14 04 0d[ 	]+comunltss 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2d7d:[ 	]+0f 25 2e 62 14 04 0e[ 	]+comunless 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2d84:[ 	]+0f 25 2e 62 14 04 0f[ 	]+comtruess 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2d8b:[ 	]+0f 25 2f e3 14 04[ 	]+comnesd %xmm3,%xmm12,%xmm1
[ 	]+2d91:[ 	]+0f 25 2f 62 14 04 04[ 	]+comnesd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2d98:[ 	]+0f 25 2f e3 14 00[ 	]+comeqsd %xmm3,%xmm12,%xmm1
[ 	]+2d9e:[ 	]+0f 25 2f e3 14 01[ 	]+comltsd %xmm3,%xmm12,%xmm1
[ 	]+2da4:[ 	]+0f 25 2f e3 14 02[ 	]+comlesd %xmm3,%xmm12,%xmm1
[ 	]+2daa:[ 	]+0f 25 2f e3 14 03[ 	]+comunordsd %xmm3,%xmm12,%xmm1
[ 	]+2db0:[ 	]+0f 25 2f e3 14 04[ 	]+comnesd %xmm3,%xmm12,%xmm1
[ 	]+2db6:[ 	]+0f 25 2f e3 14 05[ 	]+comnltsd %xmm3,%xmm12,%xmm1
[ 	]+2dbc:[ 	]+0f 25 2f e3 14 06[ 	]+comnlesd %xmm3,%xmm12,%xmm1
[ 	]+2dc2:[ 	]+0f 25 2f e3 14 07[ 	]+comordsd %xmm3,%xmm12,%xmm1
[ 	]+2dc8:[ 	]+0f 25 2f e3 14 08[ 	]+comueqsd %xmm3,%xmm12,%xmm1
[ 	]+2dce:[ 	]+0f 25 2f e3 14 09[ 	]+comultsd %xmm3,%xmm12,%xmm1
[ 	]+2dd4:[ 	]+0f 25 2f e3 14 0a[ 	]+comulesd %xmm3,%xmm12,%xmm1
[ 	]+2dda:[ 	]+0f 25 2f e3 14 0b[ 	]+comfalsesd %xmm3,%xmm12,%xmm1
[ 	]+2de0:[ 	]+0f 25 2f e3 14 0c[ 	]+comunesd %xmm3,%xmm12,%xmm1
[ 	]+2de6:[ 	]+0f 25 2f e3 14 0d[ 	]+comunltsd %xmm3,%xmm12,%xmm1
[ 	]+2dec:[ 	]+0f 25 2f e3 14 0e[ 	]+comunlesd %xmm3,%xmm12,%xmm1
[ 	]+2df2:[ 	]+0f 25 2f e3 14 0f[ 	]+comtruesd %xmm3,%xmm12,%xmm1
[ 	]+2df8:[ 	]+0f 25 2f 62 14 04 00[ 	]+comeqsd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2dff:[ 	]+0f 25 2f 62 14 04 01[ 	]+comltsd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2e06:[ 	]+0f 25 2f 62 14 04 02[ 	]+comlesd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2e0d:[ 	]+0f 25 2f 62 14 04 03[ 	]+comunordsd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2e14:[ 	]+0f 25 2f 62 14 04 04[ 	]+comnesd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2e1b:[ 	]+0f 25 2f 62 14 04 05[ 	]+comnltsd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2e22:[ 	]+0f 25 2f 62 14 04 06[ 	]+comnlesd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2e29:[ 	]+0f 25 2f 62 14 04 07[ 	]+comordsd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2e30:[ 	]+0f 25 2f 62 14 04 08[ 	]+comueqsd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2e37:[ 	]+0f 25 2f 62 14 04 09[ 	]+comultsd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2e3e:[ 	]+0f 25 2f 62 14 04 0a[ 	]+comulesd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2e45:[ 	]+0f 25 2f 62 14 04 0b[ 	]+comfalsesd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2e4c:[ 	]+0f 25 2f 62 14 04 0c[ 	]+comunesd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2e53:[ 	]+0f 25 2f 62 14 04 0d[ 	]+comunltsd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2e5a:[ 	]+0f 25 2f 62 14 04 0e[ 	]+comunlesd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2e61:[ 	]+0f 25 2f 62 14 04 0f[ 	]+comtruesd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2e68:[ 	]+0f 25 2c e3 14 04[ 	]+comneps %xmm3,%xmm12,%xmm1
[ 	]+2e6e:[ 	]+0f 25 2c 62 14 04 04[ 	]+comneps 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2e75:[ 	]+0f 25 2c e3 14 00[ 	]+comeqps %xmm3,%xmm12,%xmm1
[ 	]+2e7b:[ 	]+0f 25 2c e3 14 01[ 	]+comltps %xmm3,%xmm12,%xmm1
[ 	]+2e81:[ 	]+0f 25 2c e3 14 02[ 	]+comleps %xmm3,%xmm12,%xmm1
[ 	]+2e87:[ 	]+0f 25 2c e3 14 03[ 	]+comunordps %xmm3,%xmm12,%xmm1
[ 	]+2e8d:[ 	]+0f 25 2c e3 14 04[ 	]+comneps %xmm3,%xmm12,%xmm1
[ 	]+2e93:[ 	]+0f 25 2c e3 14 05[ 	]+comnltps %xmm3,%xmm12,%xmm1
[ 	]+2e99:[ 	]+0f 25 2c e3 14 06[ 	]+comnleps %xmm3,%xmm12,%xmm1
[ 	]+2e9f:[ 	]+0f 25 2c e3 14 07[ 	]+comordps %xmm3,%xmm12,%xmm1
[ 	]+2ea5:[ 	]+0f 25 2c e3 14 08[ 	]+comueqps %xmm3,%xmm12,%xmm1
[ 	]+2eab:[ 	]+0f 25 2c e3 14 09[ 	]+comultps %xmm3,%xmm12,%xmm1
[ 	]+2eb1:[ 	]+0f 25 2c e3 14 0a[ 	]+comuleps %xmm3,%xmm12,%xmm1
[ 	]+2eb7:[ 	]+0f 25 2c e3 14 0b[ 	]+comfalseps %xmm3,%xmm12,%xmm1
[ 	]+2ebd:[ 	]+0f 25 2c e3 14 0c[ 	]+comuneps %xmm3,%xmm12,%xmm1
[ 	]+2ec3:[ 	]+0f 25 2c e3 14 0d[ 	]+comunltps %xmm3,%xmm12,%xmm1
[ 	]+2ec9:[ 	]+0f 25 2c e3 14 0e[ 	]+comunleps %xmm3,%xmm12,%xmm1
[ 	]+2ecf:[ 	]+0f 25 2c e3 14 0f[ 	]+comtrueps %xmm3,%xmm12,%xmm1
[ 	]+2ed5:[ 	]+0f 25 2c 62 14 04 00[ 	]+comeqps 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2edc:[ 	]+0f 25 2c 62 14 04 01[ 	]+comltps 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2ee3:[ 	]+0f 25 2c 62 14 04 02[ 	]+comleps 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2eea:[ 	]+0f 25 2c 62 14 04 03[ 	]+comunordps 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2ef1:[ 	]+0f 25 2c 62 14 04 04[ 	]+comneps 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2ef8:[ 	]+0f 25 2c 62 14 04 05[ 	]+comnltps 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2eff:[ 	]+0f 25 2c 62 14 04 06[ 	]+comnleps 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2f06:[ 	]+0f 25 2c 62 14 04 07[ 	]+comordps 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2f0d:[ 	]+0f 25 2c 62 14 04 08[ 	]+comueqps 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2f14:[ 	]+0f 25 2c 62 14 04 09[ 	]+comultps 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2f1b:[ 	]+0f 25 2c 62 14 04 0a[ 	]+comuleps 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2f22:[ 	]+0f 25 2c 62 14 04 0b[ 	]+comfalseps 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2f29:[ 	]+0f 25 2c 62 14 04 0c[ 	]+comuneps 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2f30:[ 	]+0f 25 2c 62 14 04 0d[ 	]+comunltps 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2f37:[ 	]+0f 25 2c 62 14 04 0e[ 	]+comunleps 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2f3e:[ 	]+0f 25 2c 62 14 04 0f[ 	]+comtrueps 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2f45:[ 	]+0f 25 2d e3 14 04[ 	]+comnepd %xmm3,%xmm12,%xmm1
[ 	]+2f4b:[ 	]+0f 25 2d 62 14 04 04[ 	]+comnepd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2f52:[ 	]+0f 25 2d e3 14 00[ 	]+comeqpd %xmm3,%xmm12,%xmm1
[ 	]+2f58:[ 	]+0f 25 2d e3 14 01[ 	]+comltpd %xmm3,%xmm12,%xmm1
[ 	]+2f5e:[ 	]+0f 25 2d e3 14 02[ 	]+comlepd %xmm3,%xmm12,%xmm1
[ 	]+2f64:[ 	]+0f 25 2d e3 14 03[ 	]+comunordpd %xmm3,%xmm12,%xmm1
[ 	]+2f6a:[ 	]+0f 25 2d e3 14 04[ 	]+comnepd %xmm3,%xmm12,%xmm1
[ 	]+2f70:[ 	]+0f 25 2d e3 14 05[ 	]+comnltpd %xmm3,%xmm12,%xmm1
[ 	]+2f76:[ 	]+0f 25 2d e3 14 06[ 	]+comnlepd %xmm3,%xmm12,%xmm1
[ 	]+2f7c:[ 	]+0f 25 2d e3 14 07[ 	]+comordpd %xmm3,%xmm12,%xmm1
[ 	]+2f82:[ 	]+0f 25 2d e3 14 08[ 	]+comueqpd %xmm3,%xmm12,%xmm1
[ 	]+2f88:[ 	]+0f 25 2d e3 14 09[ 	]+comultpd %xmm3,%xmm12,%xmm1
[ 	]+2f8e:[ 	]+0f 25 2d e3 14 0a[ 	]+comulepd %xmm3,%xmm12,%xmm1
[ 	]+2f94:[ 	]+0f 25 2d e3 14 0b[ 	]+comfalsepd %xmm3,%xmm12,%xmm1
[ 	]+2f9a:[ 	]+0f 25 2d e3 14 0c[ 	]+comunepd %xmm3,%xmm12,%xmm1
[ 	]+2fa0:[ 	]+0f 25 2d e3 14 0d[ 	]+comunltpd %xmm3,%xmm12,%xmm1
[ 	]+2fa6:[ 	]+0f 25 2d e3 14 0e[ 	]+comunlepd %xmm3,%xmm12,%xmm1
[ 	]+2fac:[ 	]+0f 25 2d e3 14 0f[ 	]+comtruepd %xmm3,%xmm12,%xmm1
[ 	]+2fb2:[ 	]+0f 25 2d 62 14 04 00[ 	]+comeqpd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2fb9:[ 	]+0f 25 2d 62 14 04 01[ 	]+comltpd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2fc0:[ 	]+0f 25 2d 62 14 04 02[ 	]+comlepd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2fc7:[ 	]+0f 25 2d 62 14 04 03[ 	]+comunordpd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2fce:[ 	]+0f 25 2d 62 14 04 04[ 	]+comnepd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2fd5:[ 	]+0f 25 2d 62 14 04 05[ 	]+comnltpd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2fdc:[ 	]+0f 25 2d 62 14 04 06[ 	]+comnlepd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2fe3:[ 	]+0f 25 2d 62 14 04 07[ 	]+comordpd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2fea:[ 	]+0f 25 2d 62 14 04 08[ 	]+comueqpd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2ff1:[ 	]+0f 25 2d 62 14 04 09[ 	]+comultpd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2ff8:[ 	]+0f 25 2d 62 14 04 0a[ 	]+comulepd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+2fff:[ 	]+0f 25 2d 62 14 04 0b[ 	]+comfalsepd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3006:[ 	]+0f 25 2d 62 14 04 0c[ 	]+comunepd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+300d:[ 	]+0f 25 2d 62 14 04 0d[ 	]+comunltpd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3014:[ 	]+0f 25 2d 62 14 04 0e[ 	]+comunlepd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+301b:[ 	]+0f 25 2d 62 14 04 0f[ 	]+comtruepd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3022:[ 	]+0f 25 4c e3 14 04[ 	]+pcomeqb %xmm3,%xmm12,%xmm1
[ 	]+3028:[ 	]+0f 25 4c 62 14 04 04[ 	]+pcomeqb 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+302f:[ 	]+0f 25 4c e3 14 00[ 	]+pcomltb %xmm3,%xmm12,%xmm1
[ 	]+3035:[ 	]+0f 25 4c e3 14 01[ 	]+pcomleb %xmm3,%xmm12,%xmm1
[ 	]+303b:[ 	]+0f 25 4c e3 14 02[ 	]+pcomgtb %xmm3,%xmm12,%xmm1
[ 	]+3041:[ 	]+0f 25 4c e3 14 03[ 	]+pcomgeb %xmm3,%xmm12,%xmm1
[ 	]+3047:[ 	]+0f 25 4c e3 14 04[ 	]+pcomeqb %xmm3,%xmm12,%xmm1
[ 	]+304d:[ 	]+0f 25 4c e3 14 05[ 	]+pcomneb %xmm3,%xmm12,%xmm1
[ 	]+3053:[ 	]+0f 25 4c 62 14 04 00[ 	]+pcomltb 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+305a:[ 	]+0f 25 4c 62 14 04 01[ 	]+pcomleb 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3061:[ 	]+0f 25 4c 62 14 04 02[ 	]+pcomgtb 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3068:[ 	]+0f 25 4c 62 14 04 03[ 	]+pcomgeb 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+306f:[ 	]+0f 25 4c 62 14 04 04[ 	]+pcomeqb 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3076:[ 	]+0f 25 4c 62 14 04 05[ 	]+pcomneb 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+307d:[ 	]+0f 25 4d e3 14 04[ 	]+pcomeqw %xmm3,%xmm12,%xmm1
[ 	]+3083:[ 	]+0f 25 4d 62 14 04 04[ 	]+pcomeqw 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+308a:[ 	]+0f 25 4d e3 14 00[ 	]+pcomltw %xmm3,%xmm12,%xmm1
[ 	]+3090:[ 	]+0f 25 4d e3 14 01[ 	]+pcomlew %xmm3,%xmm12,%xmm1
[ 	]+3096:[ 	]+0f 25 4d e3 14 02[ 	]+pcomgtw %xmm3,%xmm12,%xmm1
[ 	]+309c:[ 	]+0f 25 4d e3 14 03[ 	]+pcomgew %xmm3,%xmm12,%xmm1
[ 	]+30a2:[ 	]+0f 25 4d e3 14 04[ 	]+pcomeqw %xmm3,%xmm12,%xmm1
[ 	]+30a8:[ 	]+0f 25 4d e3 14 05[ 	]+pcomnew %xmm3,%xmm12,%xmm1
[ 	]+30ae:[ 	]+0f 25 4d 62 14 04 00[ 	]+pcomltw 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+30b5:[ 	]+0f 25 4d 62 14 04 01[ 	]+pcomlew 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+30bc:[ 	]+0f 25 4d 62 14 04 02[ 	]+pcomgtw 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+30c3:[ 	]+0f 25 4d 62 14 04 03[ 	]+pcomgew 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+30ca:[ 	]+0f 25 4d 62 14 04 04[ 	]+pcomeqw 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+30d1:[ 	]+0f 25 4d 62 14 04 05[ 	]+pcomnew 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+30d8:[ 	]+0f 25 4e e3 14 04[ 	]+pcomeqd %xmm3,%xmm12,%xmm1
[ 	]+30de:[ 	]+0f 25 4e 62 14 04 04[ 	]+pcomeqd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+30e5:[ 	]+0f 25 4e e3 14 00[ 	]+pcomltd %xmm3,%xmm12,%xmm1
[ 	]+30eb:[ 	]+0f 25 4e e3 14 01[ 	]+pcomled %xmm3,%xmm12,%xmm1
[ 	]+30f1:[ 	]+0f 25 4e e3 14 02[ 	]+pcomgtd %xmm3,%xmm12,%xmm1
[ 	]+30f7:[ 	]+0f 25 4e e3 14 03[ 	]+pcomged %xmm3,%xmm12,%xmm1
[ 	]+30fd:[ 	]+0f 25 4e e3 14 04[ 	]+pcomeqd %xmm3,%xmm12,%xmm1
[ 	]+3103:[ 	]+0f 25 4e e3 14 05[ 	]+pcomned %xmm3,%xmm12,%xmm1
[ 	]+3109:[ 	]+0f 25 4e 62 14 04 00[ 	]+pcomltd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3110:[ 	]+0f 25 4e 62 14 04 01[ 	]+pcomled 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3117:[ 	]+0f 25 4e 62 14 04 02[ 	]+pcomgtd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+311e:[ 	]+0f 25 4e 62 14 04 03[ 	]+pcomged 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3125:[ 	]+0f 25 4e 62 14 04 04[ 	]+pcomeqd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+312c:[ 	]+0f 25 4e 62 14 04 05[ 	]+pcomned 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3133:[ 	]+0f 25 4f e3 14 04[ 	]+pcomeqq %xmm3,%xmm12,%xmm1
[ 	]+3139:[ 	]+0f 25 4f 62 14 04 04[ 	]+pcomeqq 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3140:[ 	]+0f 25 4f e3 14 00[ 	]+pcomltq %xmm3,%xmm12,%xmm1
[ 	]+3146:[ 	]+0f 25 4f e3 14 01[ 	]+pcomleq %xmm3,%xmm12,%xmm1
[ 	]+314c:[ 	]+0f 25 4f e3 14 02[ 	]+pcomgtq %xmm3,%xmm12,%xmm1
[ 	]+3152:[ 	]+0f 25 4f e3 14 03[ 	]+pcomgeq %xmm3,%xmm12,%xmm1
[ 	]+3158:[ 	]+0f 25 4f e3 14 04[ 	]+pcomeqq %xmm3,%xmm12,%xmm1
[ 	]+315e:[ 	]+0f 25 4f e3 14 05[ 	]+pcomneq %xmm3,%xmm12,%xmm1
[ 	]+3164:[ 	]+0f 25 4f 62 14 04 00[ 	]+pcomltq 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+316b:[ 	]+0f 25 4f 62 14 04 01[ 	]+pcomleq 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3172:[ 	]+0f 25 4f 62 14 04 02[ 	]+pcomgtq 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3179:[ 	]+0f 25 4f 62 14 04 03[ 	]+pcomgeq 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3180:[ 	]+0f 25 4f 62 14 04 04[ 	]+pcomeqq 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3187:[ 	]+0f 25 4f 62 14 04 05[ 	]+pcomneq 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+318e:[ 	]+0f 25 6c e3 14 04[ 	]+pcomequb %xmm3,%xmm12,%xmm1
[ 	]+3194:[ 	]+0f 25 6c 62 14 04 04[ 	]+pcomequb 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+319b:[ 	]+0f 25 6c e3 14 00[ 	]+pcomltub %xmm3,%xmm12,%xmm1
[ 	]+31a1:[ 	]+0f 25 6c e3 14 01[ 	]+pcomleub %xmm3,%xmm12,%xmm1
[ 	]+31a7:[ 	]+0f 25 6c e3 14 02[ 	]+pcomgtub %xmm3,%xmm12,%xmm1
[ 	]+31ad:[ 	]+0f 25 6c e3 14 03[ 	]+pcomgeub %xmm3,%xmm12,%xmm1
[ 	]+31b3:[ 	]+0f 25 6c e3 14 04[ 	]+pcomequb %xmm3,%xmm12,%xmm1
[ 	]+31b9:[ 	]+0f 25 6c e3 14 05[ 	]+pcomneub %xmm3,%xmm12,%xmm1
[ 	]+31bf:[ 	]+0f 25 6c 62 14 04 00[ 	]+pcomltub 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+31c6:[ 	]+0f 25 6c 62 14 04 01[ 	]+pcomleub 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+31cd:[ 	]+0f 25 6c 62 14 04 02[ 	]+pcomgtub 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+31d4:[ 	]+0f 25 6c 62 14 04 03[ 	]+pcomgeub 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+31db:[ 	]+0f 25 6c 62 14 04 04[ 	]+pcomequb 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+31e2:[ 	]+0f 25 6c 62 14 04 05[ 	]+pcomneub 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+31e9:[ 	]+0f 25 6d e3 14 04[ 	]+pcomequw %xmm3,%xmm12,%xmm1
[ 	]+31ef:[ 	]+0f 25 6d 62 14 04 04[ 	]+pcomequw 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+31f6:[ 	]+0f 25 6d e3 14 00[ 	]+pcomltuw %xmm3,%xmm12,%xmm1
[ 	]+31fc:[ 	]+0f 25 6d e3 14 01[ 	]+pcomleuw %xmm3,%xmm12,%xmm1
[ 	]+3202:[ 	]+0f 25 6d e3 14 02[ 	]+pcomgtuw %xmm3,%xmm12,%xmm1
[ 	]+3208:[ 	]+0f 25 6d e3 14 03[ 	]+pcomgeuw %xmm3,%xmm12,%xmm1
[ 	]+320e:[ 	]+0f 25 6d e3 14 04[ 	]+pcomequw %xmm3,%xmm12,%xmm1
[ 	]+3214:[ 	]+0f 25 6d e3 14 05[ 	]+pcomneuw %xmm3,%xmm12,%xmm1
[ 	]+321a:[ 	]+0f 25 6d 62 14 04 00[ 	]+pcomltuw 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3221:[ 	]+0f 25 6d 62 14 04 01[ 	]+pcomleuw 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3228:[ 	]+0f 25 6d 62 14 04 02[ 	]+pcomgtuw 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+322f:[ 	]+0f 25 6d 62 14 04 03[ 	]+pcomgeuw 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3236:[ 	]+0f 25 6d 62 14 04 04[ 	]+pcomequw 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+323d:[ 	]+0f 25 6d 62 14 04 05[ 	]+pcomneuw 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3244:[ 	]+0f 25 6e e3 14 04[ 	]+pcomequd %xmm3,%xmm12,%xmm1
[ 	]+324a:[ 	]+0f 25 6e 62 14 04 04[ 	]+pcomequd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3251:[ 	]+0f 25 6e e3 14 00[ 	]+pcomltud %xmm3,%xmm12,%xmm1
[ 	]+3257:[ 	]+0f 25 6e e3 14 01[ 	]+pcomleud %xmm3,%xmm12,%xmm1
[ 	]+325d:[ 	]+0f 25 6e e3 14 02[ 	]+pcomgtud %xmm3,%xmm12,%xmm1
[ 	]+3263:[ 	]+0f 25 6e e3 14 03[ 	]+pcomgeud %xmm3,%xmm12,%xmm1
[ 	]+3269:[ 	]+0f 25 6e e3 14 04[ 	]+pcomequd %xmm3,%xmm12,%xmm1
[ 	]+326f:[ 	]+0f 25 6e e3 14 05[ 	]+pcomneud %xmm3,%xmm12,%xmm1
[ 	]+3275:[ 	]+0f 25 6e 62 14 04 00[ 	]+pcomltud 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+327c:[ 	]+0f 25 6e 62 14 04 01[ 	]+pcomleud 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3283:[ 	]+0f 25 6e 62 14 04 02[ 	]+pcomgtud 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+328a:[ 	]+0f 25 6e 62 14 04 03[ 	]+pcomgeud 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3291:[ 	]+0f 25 6e 62 14 04 04[ 	]+pcomequd 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+3298:[ 	]+0f 25 6e 62 14 04 05[ 	]+pcomneud 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+329f:[ 	]+0f 25 6f e3 14 04[ 	]+pcomequq %xmm3,%xmm12,%xmm1
[ 	]+32a5:[ 	]+0f 25 6f 62 14 04 04[ 	]+pcomequq 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+32ac:[ 	]+0f 25 6f e3 14 00[ 	]+pcomltuq %xmm3,%xmm12,%xmm1
[ 	]+32b2:[ 	]+0f 25 6f e3 14 01[ 	]+pcomleuq %xmm3,%xmm12,%xmm1
[ 	]+32b8:[ 	]+0f 25 6f e3 14 02[ 	]+pcomgtuq %xmm3,%xmm12,%xmm1
[ 	]+32be:[ 	]+0f 25 6f e3 14 03[ 	]+pcomgeuq %xmm3,%xmm12,%xmm1
[ 	]+32c4:[ 	]+0f 25 6f e3 14 04[ 	]+pcomequq %xmm3,%xmm12,%xmm1
[ 	]+32ca:[ 	]+0f 25 6f e3 14 05[ 	]+pcomneuq %xmm3,%xmm12,%xmm1
[ 	]+32d0:[ 	]+0f 25 6f 62 14 04 00[ 	]+pcomltuq 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+32d7:[ 	]+0f 25 6f 62 14 04 01[ 	]+pcomleuq 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+32de:[ 	]+0f 25 6f 62 14 04 02[ 	]+pcomgtuq 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+32e5:[ 	]+0f 25 6f 62 14 04 03[ 	]+pcomgeuq 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+32ec:[ 	]+0f 25 6f 62 14 04 04[ 	]+pcomequq 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+32f3:[ 	]+0f 25 6f 62 14 04 05[ 	]+pcomneuq 0x4\(%rdx\),%xmm12,%xmm1
[ 	]+32fa:[ 	]+0f 25 2e d3 b0 04[ 	]+comness %xmm3,%xmm2,%xmm11
[ 	]+3300:[ 	]+0f 25 2e 52 b0 04 04[ 	]+comness 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3307:[ 	]+0f 25 2e d3 b0 00[ 	]+comeqss %xmm3,%xmm2,%xmm11
[ 	]+330d:[ 	]+0f 25 2e d3 b0 01[ 	]+comltss %xmm3,%xmm2,%xmm11
[ 	]+3313:[ 	]+0f 25 2e d3 b0 02[ 	]+comless %xmm3,%xmm2,%xmm11
[ 	]+3319:[ 	]+0f 25 2e d3 b0 03[ 	]+comunordss %xmm3,%xmm2,%xmm11
[ 	]+331f:[ 	]+0f 25 2e d3 b0 04[ 	]+comness %xmm3,%xmm2,%xmm11
[ 	]+3325:[ 	]+0f 25 2e d3 b0 05[ 	]+comnltss %xmm3,%xmm2,%xmm11
[ 	]+332b:[ 	]+0f 25 2e d3 b0 06[ 	]+comnless %xmm3,%xmm2,%xmm11
[ 	]+3331:[ 	]+0f 25 2e d3 b0 07[ 	]+comordss %xmm3,%xmm2,%xmm11
[ 	]+3337:[ 	]+0f 25 2e d3 b0 08[ 	]+comueqss %xmm3,%xmm2,%xmm11
[ 	]+333d:[ 	]+0f 25 2e d3 b0 09[ 	]+comultss %xmm3,%xmm2,%xmm11
[ 	]+3343:[ 	]+0f 25 2e d3 b0 0a[ 	]+comuless %xmm3,%xmm2,%xmm11
[ 	]+3349:[ 	]+0f 25 2e d3 b0 0b[ 	]+comfalsess %xmm3,%xmm2,%xmm11
[ 	]+334f:[ 	]+0f 25 2e d3 b0 0c[ 	]+comuness %xmm3,%xmm2,%xmm11
[ 	]+3355:[ 	]+0f 25 2e d3 b0 0d[ 	]+comunltss %xmm3,%xmm2,%xmm11
[ 	]+335b:[ 	]+0f 25 2e d3 b0 0e[ 	]+comunless %xmm3,%xmm2,%xmm11
[ 	]+3361:[ 	]+0f 25 2e d3 b0 0f[ 	]+comtruess %xmm3,%xmm2,%xmm11
[ 	]+3367:[ 	]+0f 25 2e 52 b0 04 00[ 	]+comeqss 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+336e:[ 	]+0f 25 2e 52 b0 04 01[ 	]+comltss 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3375:[ 	]+0f 25 2e 52 b0 04 02[ 	]+comless 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+337c:[ 	]+0f 25 2e 52 b0 04 03[ 	]+comunordss 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3383:[ 	]+0f 25 2e 52 b0 04 04[ 	]+comness 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+338a:[ 	]+0f 25 2e 52 b0 04 05[ 	]+comnltss 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3391:[ 	]+0f 25 2e 52 b0 04 06[ 	]+comnless 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3398:[ 	]+0f 25 2e 52 b0 04 07[ 	]+comordss 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+339f:[ 	]+0f 25 2e 52 b0 04 08[ 	]+comueqss 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+33a6:[ 	]+0f 25 2e 52 b0 04 09[ 	]+comultss 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+33ad:[ 	]+0f 25 2e 52 b0 04 0a[ 	]+comuless 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+33b4:[ 	]+0f 25 2e 52 b0 04 0b[ 	]+comfalsess 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+33bb:[ 	]+0f 25 2e 52 b0 04 0c[ 	]+comuness 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+33c2:[ 	]+0f 25 2e 52 b0 04 0d[ 	]+comunltss 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+33c9:[ 	]+0f 25 2e 52 b0 04 0e[ 	]+comunless 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+33d0:[ 	]+0f 25 2e 52 b0 04 0f[ 	]+comtruess 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+33d7:[ 	]+0f 25 2f d3 b0 04[ 	]+comnesd %xmm3,%xmm2,%xmm11
[ 	]+33dd:[ 	]+0f 25 2f 52 b0 04 04[ 	]+comnesd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+33e4:[ 	]+0f 25 2f d3 b0 00[ 	]+comeqsd %xmm3,%xmm2,%xmm11
[ 	]+33ea:[ 	]+0f 25 2f d3 b0 01[ 	]+comltsd %xmm3,%xmm2,%xmm11
[ 	]+33f0:[ 	]+0f 25 2f d3 b0 02[ 	]+comlesd %xmm3,%xmm2,%xmm11
[ 	]+33f6:[ 	]+0f 25 2f d3 b0 03[ 	]+comunordsd %xmm3,%xmm2,%xmm11
[ 	]+33fc:[ 	]+0f 25 2f d3 b0 04[ 	]+comnesd %xmm3,%xmm2,%xmm11
[ 	]+3402:[ 	]+0f 25 2f d3 b0 05[ 	]+comnltsd %xmm3,%xmm2,%xmm11
[ 	]+3408:[ 	]+0f 25 2f d3 b0 06[ 	]+comnlesd %xmm3,%xmm2,%xmm11
[ 	]+340e:[ 	]+0f 25 2f d3 b0 07[ 	]+comordsd %xmm3,%xmm2,%xmm11
[ 	]+3414:[ 	]+0f 25 2f d3 b0 08[ 	]+comueqsd %xmm3,%xmm2,%xmm11
[ 	]+341a:[ 	]+0f 25 2f d3 b0 09[ 	]+comultsd %xmm3,%xmm2,%xmm11
[ 	]+3420:[ 	]+0f 25 2f d3 b0 0a[ 	]+comulesd %xmm3,%xmm2,%xmm11
[ 	]+3426:[ 	]+0f 25 2f d3 b0 0b[ 	]+comfalsesd %xmm3,%xmm2,%xmm11
[ 	]+342c:[ 	]+0f 25 2f d3 b0 0c[ 	]+comunesd %xmm3,%xmm2,%xmm11
[ 	]+3432:[ 	]+0f 25 2f d3 b0 0d[ 	]+comunltsd %xmm3,%xmm2,%xmm11
[ 	]+3438:[ 	]+0f 25 2f d3 b0 0e[ 	]+comunlesd %xmm3,%xmm2,%xmm11
[ 	]+343e:[ 	]+0f 25 2f d3 b0 0f[ 	]+comtruesd %xmm3,%xmm2,%xmm11
[ 	]+3444:[ 	]+0f 25 2f 52 b0 04 00[ 	]+comeqsd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+344b:[ 	]+0f 25 2f 52 b0 04 01[ 	]+comltsd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3452:[ 	]+0f 25 2f 52 b0 04 02[ 	]+comlesd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3459:[ 	]+0f 25 2f 52 b0 04 03[ 	]+comunordsd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3460:[ 	]+0f 25 2f 52 b0 04 04[ 	]+comnesd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3467:[ 	]+0f 25 2f 52 b0 04 05[ 	]+comnltsd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+346e:[ 	]+0f 25 2f 52 b0 04 06[ 	]+comnlesd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3475:[ 	]+0f 25 2f 52 b0 04 07[ 	]+comordsd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+347c:[ 	]+0f 25 2f 52 b0 04 08[ 	]+comueqsd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3483:[ 	]+0f 25 2f 52 b0 04 09[ 	]+comultsd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+348a:[ 	]+0f 25 2f 52 b0 04 0a[ 	]+comulesd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3491:[ 	]+0f 25 2f 52 b0 04 0b[ 	]+comfalsesd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3498:[ 	]+0f 25 2f 52 b0 04 0c[ 	]+comunesd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+349f:[ 	]+0f 25 2f 52 b0 04 0d[ 	]+comunltsd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+34a6:[ 	]+0f 25 2f 52 b0 04 0e[ 	]+comunlesd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+34ad:[ 	]+0f 25 2f 52 b0 04 0f[ 	]+comtruesd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+34b4:[ 	]+0f 25 2c d3 b0 04[ 	]+comneps %xmm3,%xmm2,%xmm11
[ 	]+34ba:[ 	]+0f 25 2c 52 b0 04 04[ 	]+comneps 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+34c1:[ 	]+0f 25 2c d3 b0 00[ 	]+comeqps %xmm3,%xmm2,%xmm11
[ 	]+34c7:[ 	]+0f 25 2c d3 b0 01[ 	]+comltps %xmm3,%xmm2,%xmm11
[ 	]+34cd:[ 	]+0f 25 2c d3 b0 02[ 	]+comleps %xmm3,%xmm2,%xmm11
[ 	]+34d3:[ 	]+0f 25 2c d3 b0 03[ 	]+comunordps %xmm3,%xmm2,%xmm11
[ 	]+34d9:[ 	]+0f 25 2c d3 b0 04[ 	]+comneps %xmm3,%xmm2,%xmm11
[ 	]+34df:[ 	]+0f 25 2c d3 b0 05[ 	]+comnltps %xmm3,%xmm2,%xmm11
[ 	]+34e5:[ 	]+0f 25 2c d3 b0 06[ 	]+comnleps %xmm3,%xmm2,%xmm11
[ 	]+34eb:[ 	]+0f 25 2c d3 b0 07[ 	]+comordps %xmm3,%xmm2,%xmm11
[ 	]+34f1:[ 	]+0f 25 2c d3 b0 08[ 	]+comueqps %xmm3,%xmm2,%xmm11
[ 	]+34f7:[ 	]+0f 25 2c d3 b0 09[ 	]+comultps %xmm3,%xmm2,%xmm11
[ 	]+34fd:[ 	]+0f 25 2c d3 b0 0a[ 	]+comuleps %xmm3,%xmm2,%xmm11
[ 	]+3503:[ 	]+0f 25 2c d3 b0 0b[ 	]+comfalseps %xmm3,%xmm2,%xmm11
[ 	]+3509:[ 	]+0f 25 2c d3 b0 0c[ 	]+comuneps %xmm3,%xmm2,%xmm11
[ 	]+350f:[ 	]+0f 25 2c d3 b0 0d[ 	]+comunltps %xmm3,%xmm2,%xmm11
[ 	]+3515:[ 	]+0f 25 2c d3 b0 0e[ 	]+comunleps %xmm3,%xmm2,%xmm11
[ 	]+351b:[ 	]+0f 25 2c d3 b0 0f[ 	]+comtrueps %xmm3,%xmm2,%xmm11
[ 	]+3521:[ 	]+0f 25 2c 52 b0 04 00[ 	]+comeqps 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3528:[ 	]+0f 25 2c 52 b0 04 01[ 	]+comltps 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+352f:[ 	]+0f 25 2c 52 b0 04 02[ 	]+comleps 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3536:[ 	]+0f 25 2c 52 b0 04 03[ 	]+comunordps 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+353d:[ 	]+0f 25 2c 52 b0 04 04[ 	]+comneps 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3544:[ 	]+0f 25 2c 52 b0 04 05[ 	]+comnltps 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+354b:[ 	]+0f 25 2c 52 b0 04 06[ 	]+comnleps 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3552:[ 	]+0f 25 2c 52 b0 04 07[ 	]+comordps 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3559:[ 	]+0f 25 2c 52 b0 04 08[ 	]+comueqps 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3560:[ 	]+0f 25 2c 52 b0 04 09[ 	]+comultps 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3567:[ 	]+0f 25 2c 52 b0 04 0a[ 	]+comuleps 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+356e:[ 	]+0f 25 2c 52 b0 04 0b[ 	]+comfalseps 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3575:[ 	]+0f 25 2c 52 b0 04 0c[ 	]+comuneps 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+357c:[ 	]+0f 25 2c 52 b0 04 0d[ 	]+comunltps 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3583:[ 	]+0f 25 2c 52 b0 04 0e[ 	]+comunleps 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+358a:[ 	]+0f 25 2c 52 b0 04 0f[ 	]+comtrueps 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3591:[ 	]+0f 25 2d d3 b0 04[ 	]+comnepd %xmm3,%xmm2,%xmm11
[ 	]+3597:[ 	]+0f 25 2d 52 b0 04 04[ 	]+comnepd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+359e:[ 	]+0f 25 2d d3 b0 00[ 	]+comeqpd %xmm3,%xmm2,%xmm11
[ 	]+35a4:[ 	]+0f 25 2d d3 b0 01[ 	]+comltpd %xmm3,%xmm2,%xmm11
[ 	]+35aa:[ 	]+0f 25 2d d3 b0 02[ 	]+comlepd %xmm3,%xmm2,%xmm11
[ 	]+35b0:[ 	]+0f 25 2d d3 b0 03[ 	]+comunordpd %xmm3,%xmm2,%xmm11
[ 	]+35b6:[ 	]+0f 25 2d d3 b0 04[ 	]+comnepd %xmm3,%xmm2,%xmm11
[ 	]+35bc:[ 	]+0f 25 2d d3 b0 05[ 	]+comnltpd %xmm3,%xmm2,%xmm11
[ 	]+35c2:[ 	]+0f 25 2d d3 b0 06[ 	]+comnlepd %xmm3,%xmm2,%xmm11
[ 	]+35c8:[ 	]+0f 25 2d d3 b0 07[ 	]+comordpd %xmm3,%xmm2,%xmm11
[ 	]+35ce:[ 	]+0f 25 2d d3 b0 08[ 	]+comueqpd %xmm3,%xmm2,%xmm11
[ 	]+35d4:[ 	]+0f 25 2d d3 b0 09[ 	]+comultpd %xmm3,%xmm2,%xmm11
[ 	]+35da:[ 	]+0f 25 2d d3 b0 0a[ 	]+comulepd %xmm3,%xmm2,%xmm11
[ 	]+35e0:[ 	]+0f 25 2d d3 b0 0b[ 	]+comfalsepd %xmm3,%xmm2,%xmm11
[ 	]+35e6:[ 	]+0f 25 2d d3 b0 0c[ 	]+comunepd %xmm3,%xmm2,%xmm11
[ 	]+35ec:[ 	]+0f 25 2d d3 b0 0d[ 	]+comunltpd %xmm3,%xmm2,%xmm11
[ 	]+35f2:[ 	]+0f 25 2d d3 b0 0e[ 	]+comunlepd %xmm3,%xmm2,%xmm11
[ 	]+35f8:[ 	]+0f 25 2d d3 b0 0f[ 	]+comtruepd %xmm3,%xmm2,%xmm11
[ 	]+35fe:[ 	]+0f 25 2d 52 b0 04 00[ 	]+comeqpd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3605:[ 	]+0f 25 2d 52 b0 04 01[ 	]+comltpd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+360c:[ 	]+0f 25 2d 52 b0 04 02[ 	]+comlepd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3613:[ 	]+0f 25 2d 52 b0 04 03[ 	]+comunordpd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+361a:[ 	]+0f 25 2d 52 b0 04 04[ 	]+comnepd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3621:[ 	]+0f 25 2d 52 b0 04 05[ 	]+comnltpd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3628:[ 	]+0f 25 2d 52 b0 04 06[ 	]+comnlepd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+362f:[ 	]+0f 25 2d 52 b0 04 07[ 	]+comordpd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3636:[ 	]+0f 25 2d 52 b0 04 08[ 	]+comueqpd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+363d:[ 	]+0f 25 2d 52 b0 04 09[ 	]+comultpd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3644:[ 	]+0f 25 2d 52 b0 04 0a[ 	]+comulepd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+364b:[ 	]+0f 25 2d 52 b0 04 0b[ 	]+comfalsepd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3652:[ 	]+0f 25 2d 52 b0 04 0c[ 	]+comunepd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3659:[ 	]+0f 25 2d 52 b0 04 0d[ 	]+comunltpd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3660:[ 	]+0f 25 2d 52 b0 04 0e[ 	]+comunlepd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3667:[ 	]+0f 25 2d 52 b0 04 0f[ 	]+comtruepd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+366e:[ 	]+0f 25 4c d3 b0 04[ 	]+pcomeqb %xmm3,%xmm2,%xmm11
[ 	]+3674:[ 	]+0f 25 4c 52 b0 04 04[ 	]+pcomeqb 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+367b:[ 	]+0f 25 4c d3 b0 00[ 	]+pcomltb %xmm3,%xmm2,%xmm11
[ 	]+3681:[ 	]+0f 25 4c d3 b0 01[ 	]+pcomleb %xmm3,%xmm2,%xmm11
[ 	]+3687:[ 	]+0f 25 4c d3 b0 02[ 	]+pcomgtb %xmm3,%xmm2,%xmm11
[ 	]+368d:[ 	]+0f 25 4c d3 b0 03[ 	]+pcomgeb %xmm3,%xmm2,%xmm11
[ 	]+3693:[ 	]+0f 25 4c d3 b0 04[ 	]+pcomeqb %xmm3,%xmm2,%xmm11
[ 	]+3699:[ 	]+0f 25 4c d3 b0 05[ 	]+pcomneb %xmm3,%xmm2,%xmm11
[ 	]+369f:[ 	]+0f 25 4c 52 b0 04 00[ 	]+pcomltb 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+36a6:[ 	]+0f 25 4c 52 b0 04 01[ 	]+pcomleb 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+36ad:[ 	]+0f 25 4c 52 b0 04 02[ 	]+pcomgtb 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+36b4:[ 	]+0f 25 4c 52 b0 04 03[ 	]+pcomgeb 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+36bb:[ 	]+0f 25 4c 52 b0 04 04[ 	]+pcomeqb 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+36c2:[ 	]+0f 25 4c 52 b0 04 05[ 	]+pcomneb 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+36c9:[ 	]+0f 25 4d d3 b0 04[ 	]+pcomeqw %xmm3,%xmm2,%xmm11
[ 	]+36cf:[ 	]+0f 25 4d 52 b0 04 04[ 	]+pcomeqw 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+36d6:[ 	]+0f 25 4d d3 b0 00[ 	]+pcomltw %xmm3,%xmm2,%xmm11
[ 	]+36dc:[ 	]+0f 25 4d d3 b0 01[ 	]+pcomlew %xmm3,%xmm2,%xmm11
[ 	]+36e2:[ 	]+0f 25 4d d3 b0 02[ 	]+pcomgtw %xmm3,%xmm2,%xmm11
[ 	]+36e8:[ 	]+0f 25 4d d3 b0 03[ 	]+pcomgew %xmm3,%xmm2,%xmm11
[ 	]+36ee:[ 	]+0f 25 4d d3 b0 04[ 	]+pcomeqw %xmm3,%xmm2,%xmm11
[ 	]+36f4:[ 	]+0f 25 4d d3 b0 05[ 	]+pcomnew %xmm3,%xmm2,%xmm11
[ 	]+36fa:[ 	]+0f 25 4d 52 b0 04 00[ 	]+pcomltw 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3701:[ 	]+0f 25 4d 52 b0 04 01[ 	]+pcomlew 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3708:[ 	]+0f 25 4d 52 b0 04 02[ 	]+pcomgtw 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+370f:[ 	]+0f 25 4d 52 b0 04 03[ 	]+pcomgew 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3716:[ 	]+0f 25 4d 52 b0 04 04[ 	]+pcomeqw 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+371d:[ 	]+0f 25 4d 52 b0 04 05[ 	]+pcomnew 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3724:[ 	]+0f 25 4e d3 b0 04[ 	]+pcomeqd %xmm3,%xmm2,%xmm11
[ 	]+372a:[ 	]+0f 25 4e 52 b0 04 04[ 	]+pcomeqd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3731:[ 	]+0f 25 4e d3 b0 00[ 	]+pcomltd %xmm3,%xmm2,%xmm11
[ 	]+3737:[ 	]+0f 25 4e d3 b0 01[ 	]+pcomled %xmm3,%xmm2,%xmm11
[ 	]+373d:[ 	]+0f 25 4e d3 b0 02[ 	]+pcomgtd %xmm3,%xmm2,%xmm11
[ 	]+3743:[ 	]+0f 25 4e d3 b0 03[ 	]+pcomged %xmm3,%xmm2,%xmm11
[ 	]+3749:[ 	]+0f 25 4e d3 b0 04[ 	]+pcomeqd %xmm3,%xmm2,%xmm11
[ 	]+374f:[ 	]+0f 25 4e d3 b0 05[ 	]+pcomned %xmm3,%xmm2,%xmm11
[ 	]+3755:[ 	]+0f 25 4e 52 b0 04 00[ 	]+pcomltd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+375c:[ 	]+0f 25 4e 52 b0 04 01[ 	]+pcomled 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3763:[ 	]+0f 25 4e 52 b0 04 02[ 	]+pcomgtd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+376a:[ 	]+0f 25 4e 52 b0 04 03[ 	]+pcomged 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3771:[ 	]+0f 25 4e 52 b0 04 04[ 	]+pcomeqd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3778:[ 	]+0f 25 4e 52 b0 04 05[ 	]+pcomned 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+377f:[ 	]+0f 25 4f d3 b0 04[ 	]+pcomeqq %xmm3,%xmm2,%xmm11
[ 	]+3785:[ 	]+0f 25 4f 52 b0 04 04[ 	]+pcomeqq 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+378c:[ 	]+0f 25 4f d3 b0 00[ 	]+pcomltq %xmm3,%xmm2,%xmm11
[ 	]+3792:[ 	]+0f 25 4f d3 b0 01[ 	]+pcomleq %xmm3,%xmm2,%xmm11
[ 	]+3798:[ 	]+0f 25 4f d3 b0 02[ 	]+pcomgtq %xmm3,%xmm2,%xmm11
[ 	]+379e:[ 	]+0f 25 4f d3 b0 03[ 	]+pcomgeq %xmm3,%xmm2,%xmm11
[ 	]+37a4:[ 	]+0f 25 4f d3 b0 04[ 	]+pcomeqq %xmm3,%xmm2,%xmm11
[ 	]+37aa:[ 	]+0f 25 4f d3 b0 05[ 	]+pcomneq %xmm3,%xmm2,%xmm11
[ 	]+37b0:[ 	]+0f 25 4f 52 b0 04 00[ 	]+pcomltq 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+37b7:[ 	]+0f 25 4f 52 b0 04 01[ 	]+pcomleq 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+37be:[ 	]+0f 25 4f 52 b0 04 02[ 	]+pcomgtq 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+37c5:[ 	]+0f 25 4f 52 b0 04 03[ 	]+pcomgeq 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+37cc:[ 	]+0f 25 4f 52 b0 04 04[ 	]+pcomeqq 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+37d3:[ 	]+0f 25 4f 52 b0 04 05[ 	]+pcomneq 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+37da:[ 	]+0f 25 6c d3 b0 04[ 	]+pcomequb %xmm3,%xmm2,%xmm11
[ 	]+37e0:[ 	]+0f 25 6c 52 b0 04 04[ 	]+pcomequb 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+37e7:[ 	]+0f 25 6c d3 b0 00[ 	]+pcomltub %xmm3,%xmm2,%xmm11
[ 	]+37ed:[ 	]+0f 25 6c d3 b0 01[ 	]+pcomleub %xmm3,%xmm2,%xmm11
[ 	]+37f3:[ 	]+0f 25 6c d3 b0 02[ 	]+pcomgtub %xmm3,%xmm2,%xmm11
[ 	]+37f9:[ 	]+0f 25 6c d3 b0 03[ 	]+pcomgeub %xmm3,%xmm2,%xmm11
[ 	]+37ff:[ 	]+0f 25 6c d3 b0 04[ 	]+pcomequb %xmm3,%xmm2,%xmm11
[ 	]+3805:[ 	]+0f 25 6c d3 b0 05[ 	]+pcomneub %xmm3,%xmm2,%xmm11
[ 	]+380b:[ 	]+0f 25 6c 52 b0 04 00[ 	]+pcomltub 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3812:[ 	]+0f 25 6c 52 b0 04 01[ 	]+pcomleub 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3819:[ 	]+0f 25 6c 52 b0 04 02[ 	]+pcomgtub 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3820:[ 	]+0f 25 6c 52 b0 04 03[ 	]+pcomgeub 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3827:[ 	]+0f 25 6c 52 b0 04 04[ 	]+pcomequb 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+382e:[ 	]+0f 25 6c 52 b0 04 05[ 	]+pcomneub 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3835:[ 	]+0f 25 6d d3 b0 04[ 	]+pcomequw %xmm3,%xmm2,%xmm11
[ 	]+383b:[ 	]+0f 25 6d 52 b0 04 04[ 	]+pcomequw 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3842:[ 	]+0f 25 6d d3 b0 00[ 	]+pcomltuw %xmm3,%xmm2,%xmm11
[ 	]+3848:[ 	]+0f 25 6d d3 b0 01[ 	]+pcomleuw %xmm3,%xmm2,%xmm11
[ 	]+384e:[ 	]+0f 25 6d d3 b0 02[ 	]+pcomgtuw %xmm3,%xmm2,%xmm11
[ 	]+3854:[ 	]+0f 25 6d d3 b0 03[ 	]+pcomgeuw %xmm3,%xmm2,%xmm11
[ 	]+385a:[ 	]+0f 25 6d d3 b0 04[ 	]+pcomequw %xmm3,%xmm2,%xmm11
[ 	]+3860:[ 	]+0f 25 6d d3 b0 05[ 	]+pcomneuw %xmm3,%xmm2,%xmm11
[ 	]+3866:[ 	]+0f 25 6d 52 b0 04 00[ 	]+pcomltuw 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+386d:[ 	]+0f 25 6d 52 b0 04 01[ 	]+pcomleuw 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3874:[ 	]+0f 25 6d 52 b0 04 02[ 	]+pcomgtuw 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+387b:[ 	]+0f 25 6d 52 b0 04 03[ 	]+pcomgeuw 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3882:[ 	]+0f 25 6d 52 b0 04 04[ 	]+pcomequw 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3889:[ 	]+0f 25 6d 52 b0 04 05[ 	]+pcomneuw 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3890:[ 	]+0f 25 6e d3 b0 04[ 	]+pcomequd %xmm3,%xmm2,%xmm11
[ 	]+3896:[ 	]+0f 25 6e 52 b0 04 04[ 	]+pcomequd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+389d:[ 	]+0f 25 6e d3 b0 00[ 	]+pcomltud %xmm3,%xmm2,%xmm11
[ 	]+38a3:[ 	]+0f 25 6e d3 b0 01[ 	]+pcomleud %xmm3,%xmm2,%xmm11
[ 	]+38a9:[ 	]+0f 25 6e d3 b0 02[ 	]+pcomgtud %xmm3,%xmm2,%xmm11
[ 	]+38af:[ 	]+0f 25 6e d3 b0 03[ 	]+pcomgeud %xmm3,%xmm2,%xmm11
[ 	]+38b5:[ 	]+0f 25 6e d3 b0 04[ 	]+pcomequd %xmm3,%xmm2,%xmm11
[ 	]+38bb:[ 	]+0f 25 6e d3 b0 05[ 	]+pcomneud %xmm3,%xmm2,%xmm11
[ 	]+38c1:[ 	]+0f 25 6e 52 b0 04 00[ 	]+pcomltud 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+38c8:[ 	]+0f 25 6e 52 b0 04 01[ 	]+pcomleud 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+38cf:[ 	]+0f 25 6e 52 b0 04 02[ 	]+pcomgtud 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+38d6:[ 	]+0f 25 6e 52 b0 04 03[ 	]+pcomgeud 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+38dd:[ 	]+0f 25 6e 52 b0 04 04[ 	]+pcomequd 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+38e4:[ 	]+0f 25 6e 52 b0 04 05[ 	]+pcomneud 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+38eb:[ 	]+0f 25 6f d3 b0 04[ 	]+pcomequq %xmm3,%xmm2,%xmm11
[ 	]+38f1:[ 	]+0f 25 6f 52 b0 04 04[ 	]+pcomequq 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+38f8:[ 	]+0f 25 6f d3 b0 00[ 	]+pcomltuq %xmm3,%xmm2,%xmm11
[ 	]+38fe:[ 	]+0f 25 6f d3 b0 01[ 	]+pcomleuq %xmm3,%xmm2,%xmm11
[ 	]+3904:[ 	]+0f 25 6f d3 b0 02[ 	]+pcomgtuq %xmm3,%xmm2,%xmm11
[ 	]+390a:[ 	]+0f 25 6f d3 b0 03[ 	]+pcomgeuq %xmm3,%xmm2,%xmm11
[ 	]+3910:[ 	]+0f 25 6f d3 b0 04[ 	]+pcomequq %xmm3,%xmm2,%xmm11
[ 	]+3916:[ 	]+0f 25 6f d3 b0 05[ 	]+pcomneuq %xmm3,%xmm2,%xmm11
[ 	]+391c:[ 	]+0f 25 6f 52 b0 04 00[ 	]+pcomltuq 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3923:[ 	]+0f 25 6f 52 b0 04 01[ 	]+pcomleuq 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+392a:[ 	]+0f 25 6f 52 b0 04 02[ 	]+pcomgtuq 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3931:[ 	]+0f 25 6f 52 b0 04 03[ 	]+pcomgeuq 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3938:[ 	]+0f 25 6f 52 b0 04 04[ 	]+pcomequq 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+393f:[ 	]+0f 25 6f 52 b0 04 05[ 	]+pcomneuq 0x4\(%rdx\),%xmm2,%xmm11
[ 	]+3946:[ 	]+0f 25 2e d5 11 04[ 	]+comness %xmm13,%xmm2,%xmm1
[ 	]+394c:[ 	]+0f 25 2e 52 10 04 04[ 	]+comness 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3953:[ 	]+0f 25 2e d5 11 00[ 	]+comeqss %xmm13,%xmm2,%xmm1
[ 	]+3959:[ 	]+0f 25 2e d5 11 01[ 	]+comltss %xmm13,%xmm2,%xmm1
[ 	]+395f:[ 	]+0f 25 2e d5 11 02[ 	]+comless %xmm13,%xmm2,%xmm1
[ 	]+3965:[ 	]+0f 25 2e d5 11 03[ 	]+comunordss %xmm13,%xmm2,%xmm1
[ 	]+396b:[ 	]+0f 25 2e d5 11 04[ 	]+comness %xmm13,%xmm2,%xmm1
[ 	]+3971:[ 	]+0f 25 2e d5 11 05[ 	]+comnltss %xmm13,%xmm2,%xmm1
[ 	]+3977:[ 	]+0f 25 2e d5 11 06[ 	]+comnless %xmm13,%xmm2,%xmm1
[ 	]+397d:[ 	]+0f 25 2e d5 11 07[ 	]+comordss %xmm13,%xmm2,%xmm1
[ 	]+3983:[ 	]+0f 25 2e d5 11 08[ 	]+comueqss %xmm13,%xmm2,%xmm1
[ 	]+3989:[ 	]+0f 25 2e d5 11 09[ 	]+comultss %xmm13,%xmm2,%xmm1
[ 	]+398f:[ 	]+0f 25 2e d5 11 0a[ 	]+comuless %xmm13,%xmm2,%xmm1
[ 	]+3995:[ 	]+0f 25 2e d5 11 0b[ 	]+comfalsess %xmm13,%xmm2,%xmm1
[ 	]+399b:[ 	]+0f 25 2e d5 11 0c[ 	]+comuness %xmm13,%xmm2,%xmm1
[ 	]+39a1:[ 	]+0f 25 2e d5 11 0d[ 	]+comunltss %xmm13,%xmm2,%xmm1
[ 	]+39a7:[ 	]+0f 25 2e d5 11 0e[ 	]+comunless %xmm13,%xmm2,%xmm1
[ 	]+39ad:[ 	]+0f 25 2e d5 11 0f[ 	]+comtruess %xmm13,%xmm2,%xmm1
[ 	]+39b3:[ 	]+0f 25 2e 52 10 04 00[ 	]+comeqss 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+39ba:[ 	]+0f 25 2e 52 10 04 01[ 	]+comltss 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+39c1:[ 	]+0f 25 2e 52 10 04 02[ 	]+comless 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+39c8:[ 	]+0f 25 2e 52 10 04 03[ 	]+comunordss 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+39cf:[ 	]+0f 25 2e 52 10 04 04[ 	]+comness 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+39d6:[ 	]+0f 25 2e 52 10 04 05[ 	]+comnltss 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+39dd:[ 	]+0f 25 2e 52 10 04 06[ 	]+comnless 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+39e4:[ 	]+0f 25 2e 52 10 04 07[ 	]+comordss 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+39eb:[ 	]+0f 25 2e 52 10 04 08[ 	]+comueqss 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+39f2:[ 	]+0f 25 2e 52 10 04 09[ 	]+comultss 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+39f9:[ 	]+0f 25 2e 52 10 04 0a[ 	]+comuless 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3a00:[ 	]+0f 25 2e 52 10 04 0b[ 	]+comfalsess 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3a07:[ 	]+0f 25 2e 52 10 04 0c[ 	]+comuness 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3a0e:[ 	]+0f 25 2e 52 10 04 0d[ 	]+comunltss 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3a15:[ 	]+0f 25 2e 52 10 04 0e[ 	]+comunless 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3a1c:[ 	]+0f 25 2e 52 10 04 0f[ 	]+comtruess 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3a23:[ 	]+0f 25 2f d5 11 04[ 	]+comnesd %xmm13,%xmm2,%xmm1
[ 	]+3a29:[ 	]+0f 25 2f 52 10 04 04[ 	]+comnesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3a30:[ 	]+0f 25 2f d5 11 00[ 	]+comeqsd %xmm13,%xmm2,%xmm1
[ 	]+3a36:[ 	]+0f 25 2f d5 11 01[ 	]+comltsd %xmm13,%xmm2,%xmm1
[ 	]+3a3c:[ 	]+0f 25 2f d5 11 02[ 	]+comlesd %xmm13,%xmm2,%xmm1
[ 	]+3a42:[ 	]+0f 25 2f d5 11 03[ 	]+comunordsd %xmm13,%xmm2,%xmm1
[ 	]+3a48:[ 	]+0f 25 2f d5 11 04[ 	]+comnesd %xmm13,%xmm2,%xmm1
[ 	]+3a4e:[ 	]+0f 25 2f d5 11 05[ 	]+comnltsd %xmm13,%xmm2,%xmm1
[ 	]+3a54:[ 	]+0f 25 2f d5 11 06[ 	]+comnlesd %xmm13,%xmm2,%xmm1
[ 	]+3a5a:[ 	]+0f 25 2f d5 11 07[ 	]+comordsd %xmm13,%xmm2,%xmm1
[ 	]+3a60:[ 	]+0f 25 2f d5 11 08[ 	]+comueqsd %xmm13,%xmm2,%xmm1
[ 	]+3a66:[ 	]+0f 25 2f d5 11 09[ 	]+comultsd %xmm13,%xmm2,%xmm1
[ 	]+3a6c:[ 	]+0f 25 2f d5 11 0a[ 	]+comulesd %xmm13,%xmm2,%xmm1
[ 	]+3a72:[ 	]+0f 25 2f d5 11 0b[ 	]+comfalsesd %xmm13,%xmm2,%xmm1
[ 	]+3a78:[ 	]+0f 25 2f d5 11 0c[ 	]+comunesd %xmm13,%xmm2,%xmm1
[ 	]+3a7e:[ 	]+0f 25 2f d5 11 0d[ 	]+comunltsd %xmm13,%xmm2,%xmm1
[ 	]+3a84:[ 	]+0f 25 2f d5 11 0e[ 	]+comunlesd %xmm13,%xmm2,%xmm1
[ 	]+3a8a:[ 	]+0f 25 2f d5 11 0f[ 	]+comtruesd %xmm13,%xmm2,%xmm1
[ 	]+3a90:[ 	]+0f 25 2f 52 10 04 00[ 	]+comeqsd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3a97:[ 	]+0f 25 2f 52 10 04 01[ 	]+comltsd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3a9e:[ 	]+0f 25 2f 52 10 04 02[ 	]+comlesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3aa5:[ 	]+0f 25 2f 52 10 04 03[ 	]+comunordsd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3aac:[ 	]+0f 25 2f 52 10 04 04[ 	]+comnesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3ab3:[ 	]+0f 25 2f 52 10 04 05[ 	]+comnltsd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3aba:[ 	]+0f 25 2f 52 10 04 06[ 	]+comnlesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3ac1:[ 	]+0f 25 2f 52 10 04 07[ 	]+comordsd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3ac8:[ 	]+0f 25 2f 52 10 04 08[ 	]+comueqsd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3acf:[ 	]+0f 25 2f 52 10 04 09[ 	]+comultsd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3ad6:[ 	]+0f 25 2f 52 10 04 0a[ 	]+comulesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3add:[ 	]+0f 25 2f 52 10 04 0b[ 	]+comfalsesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3ae4:[ 	]+0f 25 2f 52 10 04 0c[ 	]+comunesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3aeb:[ 	]+0f 25 2f 52 10 04 0d[ 	]+comunltsd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3af2:[ 	]+0f 25 2f 52 10 04 0e[ 	]+comunlesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3af9:[ 	]+0f 25 2f 52 10 04 0f[ 	]+comtruesd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3b00:[ 	]+0f 25 2c d5 11 04[ 	]+comneps %xmm13,%xmm2,%xmm1
[ 	]+3b06:[ 	]+0f 25 2c 52 10 04 04[ 	]+comneps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3b0d:[ 	]+0f 25 2c d5 11 00[ 	]+comeqps %xmm13,%xmm2,%xmm1
[ 	]+3b13:[ 	]+0f 25 2c d5 11 01[ 	]+comltps %xmm13,%xmm2,%xmm1
[ 	]+3b19:[ 	]+0f 25 2c d5 11 02[ 	]+comleps %xmm13,%xmm2,%xmm1
[ 	]+3b1f:[ 	]+0f 25 2c d5 11 03[ 	]+comunordps %xmm13,%xmm2,%xmm1
[ 	]+3b25:[ 	]+0f 25 2c d5 11 04[ 	]+comneps %xmm13,%xmm2,%xmm1
[ 	]+3b2b:[ 	]+0f 25 2c d5 11 05[ 	]+comnltps %xmm13,%xmm2,%xmm1
[ 	]+3b31:[ 	]+0f 25 2c d5 11 06[ 	]+comnleps %xmm13,%xmm2,%xmm1
[ 	]+3b37:[ 	]+0f 25 2c d5 11 07[ 	]+comordps %xmm13,%xmm2,%xmm1
[ 	]+3b3d:[ 	]+0f 25 2c d5 11 08[ 	]+comueqps %xmm13,%xmm2,%xmm1
[ 	]+3b43:[ 	]+0f 25 2c d5 11 09[ 	]+comultps %xmm13,%xmm2,%xmm1
[ 	]+3b49:[ 	]+0f 25 2c d5 11 0a[ 	]+comuleps %xmm13,%xmm2,%xmm1
[ 	]+3b4f:[ 	]+0f 25 2c d5 11 0b[ 	]+comfalseps %xmm13,%xmm2,%xmm1
[ 	]+3b55:[ 	]+0f 25 2c d5 11 0c[ 	]+comuneps %xmm13,%xmm2,%xmm1
[ 	]+3b5b:[ 	]+0f 25 2c d5 11 0d[ 	]+comunltps %xmm13,%xmm2,%xmm1
[ 	]+3b61:[ 	]+0f 25 2c d5 11 0e[ 	]+comunleps %xmm13,%xmm2,%xmm1
[ 	]+3b67:[ 	]+0f 25 2c d5 11 0f[ 	]+comtrueps %xmm13,%xmm2,%xmm1
[ 	]+3b6d:[ 	]+0f 25 2c 52 10 04 00[ 	]+comeqps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3b74:[ 	]+0f 25 2c 52 10 04 01[ 	]+comltps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3b7b:[ 	]+0f 25 2c 52 10 04 02[ 	]+comleps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3b82:[ 	]+0f 25 2c 52 10 04 03[ 	]+comunordps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3b89:[ 	]+0f 25 2c 52 10 04 04[ 	]+comneps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3b90:[ 	]+0f 25 2c 52 10 04 05[ 	]+comnltps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3b97:[ 	]+0f 25 2c 52 10 04 06[ 	]+comnleps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3b9e:[ 	]+0f 25 2c 52 10 04 07[ 	]+comordps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3ba5:[ 	]+0f 25 2c 52 10 04 08[ 	]+comueqps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3bac:[ 	]+0f 25 2c 52 10 04 09[ 	]+comultps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3bb3:[ 	]+0f 25 2c 52 10 04 0a[ 	]+comuleps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3bba:[ 	]+0f 25 2c 52 10 04 0b[ 	]+comfalseps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3bc1:[ 	]+0f 25 2c 52 10 04 0c[ 	]+comuneps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3bc8:[ 	]+0f 25 2c 52 10 04 0d[ 	]+comunltps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3bcf:[ 	]+0f 25 2c 52 10 04 0e[ 	]+comunleps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3bd6:[ 	]+0f 25 2c 52 10 04 0f[ 	]+comtrueps 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3bdd:[ 	]+0f 25 2d d5 11 04[ 	]+comnepd %xmm13,%xmm2,%xmm1
[ 	]+3be3:[ 	]+0f 25 2d 52 10 04 04[ 	]+comnepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3bea:[ 	]+0f 25 2d d5 11 00[ 	]+comeqpd %xmm13,%xmm2,%xmm1
[ 	]+3bf0:[ 	]+0f 25 2d d5 11 01[ 	]+comltpd %xmm13,%xmm2,%xmm1
[ 	]+3bf6:[ 	]+0f 25 2d d5 11 02[ 	]+comlepd %xmm13,%xmm2,%xmm1
[ 	]+3bfc:[ 	]+0f 25 2d d5 11 03[ 	]+comunordpd %xmm13,%xmm2,%xmm1
[ 	]+3c02:[ 	]+0f 25 2d d5 11 04[ 	]+comnepd %xmm13,%xmm2,%xmm1
[ 	]+3c08:[ 	]+0f 25 2d d5 11 05[ 	]+comnltpd %xmm13,%xmm2,%xmm1
[ 	]+3c0e:[ 	]+0f 25 2d d5 11 06[ 	]+comnlepd %xmm13,%xmm2,%xmm1
[ 	]+3c14:[ 	]+0f 25 2d d5 11 07[ 	]+comordpd %xmm13,%xmm2,%xmm1
[ 	]+3c1a:[ 	]+0f 25 2d d5 11 08[ 	]+comueqpd %xmm13,%xmm2,%xmm1
[ 	]+3c20:[ 	]+0f 25 2d d5 11 09[ 	]+comultpd %xmm13,%xmm2,%xmm1
[ 	]+3c26:[ 	]+0f 25 2d d5 11 0a[ 	]+comulepd %xmm13,%xmm2,%xmm1
[ 	]+3c2c:[ 	]+0f 25 2d d5 11 0b[ 	]+comfalsepd %xmm13,%xmm2,%xmm1
[ 	]+3c32:[ 	]+0f 25 2d d5 11 0c[ 	]+comunepd %xmm13,%xmm2,%xmm1
[ 	]+3c38:[ 	]+0f 25 2d d5 11 0d[ 	]+comunltpd %xmm13,%xmm2,%xmm1
[ 	]+3c3e:[ 	]+0f 25 2d d5 11 0e[ 	]+comunlepd %xmm13,%xmm2,%xmm1
[ 	]+3c44:[ 	]+0f 25 2d d5 11 0f[ 	]+comtruepd %xmm13,%xmm2,%xmm1
[ 	]+3c4a:[ 	]+0f 25 2d 52 10 04 00[ 	]+comeqpd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3c51:[ 	]+0f 25 2d 52 10 04 01[ 	]+comltpd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3c58:[ 	]+0f 25 2d 52 10 04 02[ 	]+comlepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3c5f:[ 	]+0f 25 2d 52 10 04 03[ 	]+comunordpd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3c66:[ 	]+0f 25 2d 52 10 04 04[ 	]+comnepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3c6d:[ 	]+0f 25 2d 52 10 04 05[ 	]+comnltpd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3c74:[ 	]+0f 25 2d 52 10 04 06[ 	]+comnlepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3c7b:[ 	]+0f 25 2d 52 10 04 07[ 	]+comordpd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3c82:[ 	]+0f 25 2d 52 10 04 08[ 	]+comueqpd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3c89:[ 	]+0f 25 2d 52 10 04 09[ 	]+comultpd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3c90:[ 	]+0f 25 2d 52 10 04 0a[ 	]+comulepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3c97:[ 	]+0f 25 2d 52 10 04 0b[ 	]+comfalsepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3c9e:[ 	]+0f 25 2d 52 10 04 0c[ 	]+comunepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3ca5:[ 	]+0f 25 2d 52 10 04 0d[ 	]+comunltpd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3cac:[ 	]+0f 25 2d 52 10 04 0e[ 	]+comunlepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3cb3:[ 	]+0f 25 2d 52 10 04 0f[ 	]+comtruepd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3cba:[ 	]+0f 25 4c d5 11 04[ 	]+pcomeqb %xmm13,%xmm2,%xmm1
[ 	]+3cc0:[ 	]+0f 25 4c 52 10 04 04[ 	]+pcomeqb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3cc7:[ 	]+0f 25 4c d5 11 00[ 	]+pcomltb %xmm13,%xmm2,%xmm1
[ 	]+3ccd:[ 	]+0f 25 4c d5 11 01[ 	]+pcomleb %xmm13,%xmm2,%xmm1
[ 	]+3cd3:[ 	]+0f 25 4c d5 11 02[ 	]+pcomgtb %xmm13,%xmm2,%xmm1
[ 	]+3cd9:[ 	]+0f 25 4c d5 11 03[ 	]+pcomgeb %xmm13,%xmm2,%xmm1
[ 	]+3cdf:[ 	]+0f 25 4c d5 11 04[ 	]+pcomeqb %xmm13,%xmm2,%xmm1
[ 	]+3ce5:[ 	]+0f 25 4c d5 11 05[ 	]+pcomneb %xmm13,%xmm2,%xmm1
[ 	]+3ceb:[ 	]+0f 25 4c 52 10 04 00[ 	]+pcomltb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3cf2:[ 	]+0f 25 4c 52 10 04 01[ 	]+pcomleb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3cf9:[ 	]+0f 25 4c 52 10 04 02[ 	]+pcomgtb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3d00:[ 	]+0f 25 4c 52 10 04 03[ 	]+pcomgeb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3d07:[ 	]+0f 25 4c 52 10 04 04[ 	]+pcomeqb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3d0e:[ 	]+0f 25 4c 52 10 04 05[ 	]+pcomneb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3d15:[ 	]+0f 25 4d d5 11 04[ 	]+pcomeqw %xmm13,%xmm2,%xmm1
[ 	]+3d1b:[ 	]+0f 25 4d 52 10 04 04[ 	]+pcomeqw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3d22:[ 	]+0f 25 4d d5 11 00[ 	]+pcomltw %xmm13,%xmm2,%xmm1
[ 	]+3d28:[ 	]+0f 25 4d d5 11 01[ 	]+pcomlew %xmm13,%xmm2,%xmm1
[ 	]+3d2e:[ 	]+0f 25 4d d5 11 02[ 	]+pcomgtw %xmm13,%xmm2,%xmm1
[ 	]+3d34:[ 	]+0f 25 4d d5 11 03[ 	]+pcomgew %xmm13,%xmm2,%xmm1
[ 	]+3d3a:[ 	]+0f 25 4d d5 11 04[ 	]+pcomeqw %xmm13,%xmm2,%xmm1
[ 	]+3d40:[ 	]+0f 25 4d d5 11 05[ 	]+pcomnew %xmm13,%xmm2,%xmm1
[ 	]+3d46:[ 	]+0f 25 4d 52 10 04 00[ 	]+pcomltw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3d4d:[ 	]+0f 25 4d 52 10 04 01[ 	]+pcomlew 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3d54:[ 	]+0f 25 4d 52 10 04 02[ 	]+pcomgtw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3d5b:[ 	]+0f 25 4d 52 10 04 03[ 	]+pcomgew 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3d62:[ 	]+0f 25 4d 52 10 04 04[ 	]+pcomeqw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3d69:[ 	]+0f 25 4d 52 10 04 05[ 	]+pcomnew 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3d70:[ 	]+0f 25 4e d5 11 04[ 	]+pcomeqd %xmm13,%xmm2,%xmm1
[ 	]+3d76:[ 	]+0f 25 4e 52 10 04 04[ 	]+pcomeqd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3d7d:[ 	]+0f 25 4e d5 11 00[ 	]+pcomltd %xmm13,%xmm2,%xmm1
[ 	]+3d83:[ 	]+0f 25 4e d5 11 01[ 	]+pcomled %xmm13,%xmm2,%xmm1
[ 	]+3d89:[ 	]+0f 25 4e d5 11 02[ 	]+pcomgtd %xmm13,%xmm2,%xmm1
[ 	]+3d8f:[ 	]+0f 25 4e d5 11 03[ 	]+pcomged %xmm13,%xmm2,%xmm1
[ 	]+3d95:[ 	]+0f 25 4e d5 11 04[ 	]+pcomeqd %xmm13,%xmm2,%xmm1
[ 	]+3d9b:[ 	]+0f 25 4e d5 11 05[ 	]+pcomned %xmm13,%xmm2,%xmm1
[ 	]+3da1:[ 	]+0f 25 4e 52 10 04 00[ 	]+pcomltd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3da8:[ 	]+0f 25 4e 52 10 04 01[ 	]+pcomled 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3daf:[ 	]+0f 25 4e 52 10 04 02[ 	]+pcomgtd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3db6:[ 	]+0f 25 4e 52 10 04 03[ 	]+pcomged 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3dbd:[ 	]+0f 25 4e 52 10 04 04[ 	]+pcomeqd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3dc4:[ 	]+0f 25 4e 52 10 04 05[ 	]+pcomned 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3dcb:[ 	]+0f 25 4f d5 11 04[ 	]+pcomeqq %xmm13,%xmm2,%xmm1
[ 	]+3dd1:[ 	]+0f 25 4f 52 10 04 04[ 	]+pcomeqq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3dd8:[ 	]+0f 25 4f d5 11 00[ 	]+pcomltq %xmm13,%xmm2,%xmm1
[ 	]+3dde:[ 	]+0f 25 4f d5 11 01[ 	]+pcomleq %xmm13,%xmm2,%xmm1
[ 	]+3de4:[ 	]+0f 25 4f d5 11 02[ 	]+pcomgtq %xmm13,%xmm2,%xmm1
[ 	]+3dea:[ 	]+0f 25 4f d5 11 03[ 	]+pcomgeq %xmm13,%xmm2,%xmm1
[ 	]+3df0:[ 	]+0f 25 4f d5 11 04[ 	]+pcomeqq %xmm13,%xmm2,%xmm1
[ 	]+3df6:[ 	]+0f 25 4f d5 11 05[ 	]+pcomneq %xmm13,%xmm2,%xmm1
[ 	]+3dfc:[ 	]+0f 25 4f 52 10 04 00[ 	]+pcomltq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3e03:[ 	]+0f 25 4f 52 10 04 01[ 	]+pcomleq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3e0a:[ 	]+0f 25 4f 52 10 04 02[ 	]+pcomgtq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3e11:[ 	]+0f 25 4f 52 10 04 03[ 	]+pcomgeq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3e18:[ 	]+0f 25 4f 52 10 04 04[ 	]+pcomeqq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3e1f:[ 	]+0f 25 4f 52 10 04 05[ 	]+pcomneq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3e26:[ 	]+0f 25 6c d5 11 04[ 	]+pcomequb %xmm13,%xmm2,%xmm1
[ 	]+3e2c:[ 	]+0f 25 6c 52 10 04 04[ 	]+pcomequb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3e33:[ 	]+0f 25 6c d5 11 00[ 	]+pcomltub %xmm13,%xmm2,%xmm1
[ 	]+3e39:[ 	]+0f 25 6c d5 11 01[ 	]+pcomleub %xmm13,%xmm2,%xmm1
[ 	]+3e3f:[ 	]+0f 25 6c d5 11 02[ 	]+pcomgtub %xmm13,%xmm2,%xmm1
[ 	]+3e45:[ 	]+0f 25 6c d5 11 03[ 	]+pcomgeub %xmm13,%xmm2,%xmm1
[ 	]+3e4b:[ 	]+0f 25 6c d5 11 04[ 	]+pcomequb %xmm13,%xmm2,%xmm1
[ 	]+3e51:[ 	]+0f 25 6c d5 11 05[ 	]+pcomneub %xmm13,%xmm2,%xmm1
[ 	]+3e57:[ 	]+0f 25 6c 52 10 04 00[ 	]+pcomltub 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3e5e:[ 	]+0f 25 6c 52 10 04 01[ 	]+pcomleub 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3e65:[ 	]+0f 25 6c 52 10 04 02[ 	]+pcomgtub 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3e6c:[ 	]+0f 25 6c 52 10 04 03[ 	]+pcomgeub 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3e73:[ 	]+0f 25 6c 52 10 04 04[ 	]+pcomequb 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3e7a:[ 	]+0f 25 6c 52 10 04 05[ 	]+pcomneub 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3e81:[ 	]+0f 25 6d d5 11 04[ 	]+pcomequw %xmm13,%xmm2,%xmm1
[ 	]+3e87:[ 	]+0f 25 6d 52 10 04 04[ 	]+pcomequw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3e8e:[ 	]+0f 25 6d d5 11 00[ 	]+pcomltuw %xmm13,%xmm2,%xmm1
[ 	]+3e94:[ 	]+0f 25 6d d5 11 01[ 	]+pcomleuw %xmm13,%xmm2,%xmm1
[ 	]+3e9a:[ 	]+0f 25 6d d5 11 02[ 	]+pcomgtuw %xmm13,%xmm2,%xmm1
[ 	]+3ea0:[ 	]+0f 25 6d d5 11 03[ 	]+pcomgeuw %xmm13,%xmm2,%xmm1
[ 	]+3ea6:[ 	]+0f 25 6d d5 11 04[ 	]+pcomequw %xmm13,%xmm2,%xmm1
[ 	]+3eac:[ 	]+0f 25 6d d5 11 05[ 	]+pcomneuw %xmm13,%xmm2,%xmm1
[ 	]+3eb2:[ 	]+0f 25 6d 52 10 04 00[ 	]+pcomltuw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3eb9:[ 	]+0f 25 6d 52 10 04 01[ 	]+pcomleuw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3ec0:[ 	]+0f 25 6d 52 10 04 02[ 	]+pcomgtuw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3ec7:[ 	]+0f 25 6d 52 10 04 03[ 	]+pcomgeuw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3ece:[ 	]+0f 25 6d 52 10 04 04[ 	]+pcomequw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3ed5:[ 	]+0f 25 6d 52 10 04 05[ 	]+pcomneuw 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3edc:[ 	]+0f 25 6e d5 11 04[ 	]+pcomequd %xmm13,%xmm2,%xmm1
[ 	]+3ee2:[ 	]+0f 25 6e 52 10 04 04[ 	]+pcomequd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3ee9:[ 	]+0f 25 6e d5 11 00[ 	]+pcomltud %xmm13,%xmm2,%xmm1
[ 	]+3eef:[ 	]+0f 25 6e d5 11 01[ 	]+pcomleud %xmm13,%xmm2,%xmm1
[ 	]+3ef5:[ 	]+0f 25 6e d5 11 02[ 	]+pcomgtud %xmm13,%xmm2,%xmm1
[ 	]+3efb:[ 	]+0f 25 6e d5 11 03[ 	]+pcomgeud %xmm13,%xmm2,%xmm1
[ 	]+3f01:[ 	]+0f 25 6e d5 11 04[ 	]+pcomequd %xmm13,%xmm2,%xmm1
[ 	]+3f07:[ 	]+0f 25 6e d5 11 05[ 	]+pcomneud %xmm13,%xmm2,%xmm1
[ 	]+3f0d:[ 	]+0f 25 6e 52 10 04 00[ 	]+pcomltud 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3f14:[ 	]+0f 25 6e 52 10 04 01[ 	]+pcomleud 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3f1b:[ 	]+0f 25 6e 52 10 04 02[ 	]+pcomgtud 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3f22:[ 	]+0f 25 6e 52 10 04 03[ 	]+pcomgeud 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3f29:[ 	]+0f 25 6e 52 10 04 04[ 	]+pcomequd 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3f30:[ 	]+0f 25 6e 52 10 04 05[ 	]+pcomneud 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3f37:[ 	]+0f 25 6f d5 11 04[ 	]+pcomequq %xmm13,%xmm2,%xmm1
[ 	]+3f3d:[ 	]+0f 25 6f 52 10 04 04[ 	]+pcomequq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3f44:[ 	]+0f 25 6f d5 11 00[ 	]+pcomltuq %xmm13,%xmm2,%xmm1
[ 	]+3f4a:[ 	]+0f 25 6f d5 11 01[ 	]+pcomleuq %xmm13,%xmm2,%xmm1
[ 	]+3f50:[ 	]+0f 25 6f d5 11 02[ 	]+pcomgtuq %xmm13,%xmm2,%xmm1
[ 	]+3f56:[ 	]+0f 25 6f d5 11 03[ 	]+pcomgeuq %xmm13,%xmm2,%xmm1
[ 	]+3f5c:[ 	]+0f 25 6f d5 11 04[ 	]+pcomequq %xmm13,%xmm2,%xmm1
[ 	]+3f62:[ 	]+0f 25 6f d5 11 05[ 	]+pcomneuq %xmm13,%xmm2,%xmm1
[ 	]+3f68:[ 	]+0f 25 6f 52 10 04 00[ 	]+pcomltuq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3f6f:[ 	]+0f 25 6f 52 10 04 01[ 	]+pcomleuq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3f76:[ 	]+0f 25 6f 52 10 04 02[ 	]+pcomgtuq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3f7d:[ 	]+0f 25 6f 52 10 04 03[ 	]+pcomgeuq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3f84:[ 	]+0f 25 6f 52 10 04 04[ 	]+pcomequq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3f8b:[ 	]+0f 25 6f 52 10 04 05[ 	]+pcomneuq 0x4\(%rdx\),%xmm2,%xmm1
[ 	]+3f92:[ 	]+0f 25 2e d3 10 04[ 	]+comness %xmm3,%xmm2,%xmm1
[ 	]+3f98:[ 	]+0f 25 2e 97 11 00 00 10 00 04[ 	]+comness 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+3fa2:[ 	]+0f 25 2e d3 10 00[ 	]+comeqss %xmm3,%xmm2,%xmm1
[ 	]+3fa8:[ 	]+0f 25 2e d3 10 01[ 	]+comltss %xmm3,%xmm2,%xmm1
[ 	]+3fae:[ 	]+0f 25 2e d3 10 02[ 	]+comless %xmm3,%xmm2,%xmm1
[ 	]+3fb4:[ 	]+0f 25 2e d3 10 03[ 	]+comunordss %xmm3,%xmm2,%xmm1
[ 	]+3fba:[ 	]+0f 25 2e d3 10 04[ 	]+comness %xmm3,%xmm2,%xmm1
[ 	]+3fc0:[ 	]+0f 25 2e d3 10 05[ 	]+comnltss %xmm3,%xmm2,%xmm1
[ 	]+3fc6:[ 	]+0f 25 2e d3 10 06[ 	]+comnless %xmm3,%xmm2,%xmm1
[ 	]+3fcc:[ 	]+0f 25 2e d3 10 07[ 	]+comordss %xmm3,%xmm2,%xmm1
[ 	]+3fd2:[ 	]+0f 25 2e d3 10 08[ 	]+comueqss %xmm3,%xmm2,%xmm1
[ 	]+3fd8:[ 	]+0f 25 2e d3 10 09[ 	]+comultss %xmm3,%xmm2,%xmm1
[ 	]+3fde:[ 	]+0f 25 2e d3 10 0a[ 	]+comuless %xmm3,%xmm2,%xmm1
[ 	]+3fe4:[ 	]+0f 25 2e d3 10 0b[ 	]+comfalsess %xmm3,%xmm2,%xmm1
[ 	]+3fea:[ 	]+0f 25 2e d3 10 0c[ 	]+comuness %xmm3,%xmm2,%xmm1
[ 	]+3ff0:[ 	]+0f 25 2e d3 10 0d[ 	]+comunltss %xmm3,%xmm2,%xmm1
[ 	]+3ff6:[ 	]+0f 25 2e d3 10 0e[ 	]+comunless %xmm3,%xmm2,%xmm1
[ 	]+3ffc:[ 	]+0f 25 2e d3 10 0f[ 	]+comtruess %xmm3,%xmm2,%xmm1
[ 	]+4002:[ 	]+0f 25 2e 97 11 00 00 10 00 00[ 	]+comeqss 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+400c:[ 	]+0f 25 2e 97 11 00 00 10 00 01[ 	]+comltss 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4016:[ 	]+0f 25 2e 97 11 00 00 10 00 02[ 	]+comless 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4020:[ 	]+0f 25 2e 97 11 00 00 10 00 03[ 	]+comunordss 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+402a:[ 	]+0f 25 2e 97 11 00 00 10 00 04[ 	]+comness 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4034:[ 	]+0f 25 2e 97 11 00 00 10 00 05[ 	]+comnltss 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+403e:[ 	]+0f 25 2e 97 11 00 00 10 00 06[ 	]+comnless 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4048:[ 	]+0f 25 2e 97 11 00 00 10 00 07[ 	]+comordss 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4052:[ 	]+0f 25 2e 97 11 00 00 10 00 08[ 	]+comueqss 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+405c:[ 	]+0f 25 2e 97 11 00 00 10 00 09[ 	]+comultss 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4066:[ 	]+0f 25 2e 97 11 00 00 10 00 0a[ 	]+comuless 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4070:[ 	]+0f 25 2e 97 11 00 00 10 00 0b[ 	]+comfalsess 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+407a:[ 	]+0f 25 2e 97 11 00 00 10 00 0c[ 	]+comuness 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4084:[ 	]+0f 25 2e 97 11 00 00 10 00 0d[ 	]+comunltss 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+408e:[ 	]+0f 25 2e 97 11 00 00 10 00 0e[ 	]+comunless 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4098:[ 	]+0f 25 2e 97 11 00 00 10 00 0f[ 	]+comtruess 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+40a2:[ 	]+0f 25 2f d3 10 04[ 	]+comnesd %xmm3,%xmm2,%xmm1
[ 	]+40a8:[ 	]+0f 25 2f 97 11 00 00 10 00 04[ 	]+comnesd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+40b2:[ 	]+0f 25 2f d3 10 00[ 	]+comeqsd %xmm3,%xmm2,%xmm1
[ 	]+40b8:[ 	]+0f 25 2f d3 10 01[ 	]+comltsd %xmm3,%xmm2,%xmm1
[ 	]+40be:[ 	]+0f 25 2f d3 10 02[ 	]+comlesd %xmm3,%xmm2,%xmm1
[ 	]+40c4:[ 	]+0f 25 2f d3 10 03[ 	]+comunordsd %xmm3,%xmm2,%xmm1
[ 	]+40ca:[ 	]+0f 25 2f d3 10 04[ 	]+comnesd %xmm3,%xmm2,%xmm1
[ 	]+40d0:[ 	]+0f 25 2f d3 10 05[ 	]+comnltsd %xmm3,%xmm2,%xmm1
[ 	]+40d6:[ 	]+0f 25 2f d3 10 06[ 	]+comnlesd %xmm3,%xmm2,%xmm1
[ 	]+40dc:[ 	]+0f 25 2f d3 10 07[ 	]+comordsd %xmm3,%xmm2,%xmm1
[ 	]+40e2:[ 	]+0f 25 2f d3 10 08[ 	]+comueqsd %xmm3,%xmm2,%xmm1
[ 	]+40e8:[ 	]+0f 25 2f d3 10 09[ 	]+comultsd %xmm3,%xmm2,%xmm1
[ 	]+40ee:[ 	]+0f 25 2f d3 10 0a[ 	]+comulesd %xmm3,%xmm2,%xmm1
[ 	]+40f4:[ 	]+0f 25 2f d3 10 0b[ 	]+comfalsesd %xmm3,%xmm2,%xmm1
[ 	]+40fa:[ 	]+0f 25 2f d3 10 0c[ 	]+comunesd %xmm3,%xmm2,%xmm1
[ 	]+4100:[ 	]+0f 25 2f d3 10 0d[ 	]+comunltsd %xmm3,%xmm2,%xmm1
[ 	]+4106:[ 	]+0f 25 2f d3 10 0e[ 	]+comunlesd %xmm3,%xmm2,%xmm1
[ 	]+410c:[ 	]+0f 25 2f d3 10 0f[ 	]+comtruesd %xmm3,%xmm2,%xmm1
[ 	]+4112:[ 	]+0f 25 2f 97 11 00 00 10 00 00[ 	]+comeqsd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+411c:[ 	]+0f 25 2f 97 11 00 00 10 00 01[ 	]+comltsd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4126:[ 	]+0f 25 2f 97 11 00 00 10 00 02[ 	]+comlesd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4130:[ 	]+0f 25 2f 97 11 00 00 10 00 03[ 	]+comunordsd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+413a:[ 	]+0f 25 2f 97 11 00 00 10 00 04[ 	]+comnesd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4144:[ 	]+0f 25 2f 97 11 00 00 10 00 05[ 	]+comnltsd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+414e:[ 	]+0f 25 2f 97 11 00 00 10 00 06[ 	]+comnlesd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4158:[ 	]+0f 25 2f 97 11 00 00 10 00 07[ 	]+comordsd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4162:[ 	]+0f 25 2f 97 11 00 00 10 00 08[ 	]+comueqsd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+416c:[ 	]+0f 25 2f 97 11 00 00 10 00 09[ 	]+comultsd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4176:[ 	]+0f 25 2f 97 11 00 00 10 00 0a[ 	]+comulesd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4180:[ 	]+0f 25 2f 97 11 00 00 10 00 0b[ 	]+comfalsesd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+418a:[ 	]+0f 25 2f 97 11 00 00 10 00 0c[ 	]+comunesd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4194:[ 	]+0f 25 2f 97 11 00 00 10 00 0d[ 	]+comunltsd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+419e:[ 	]+0f 25 2f 97 11 00 00 10 00 0e[ 	]+comunlesd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+41a8:[ 	]+0f 25 2f 97 11 00 00 10 00 0f[ 	]+comtruesd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+41b2:[ 	]+0f 25 2c d3 10 04[ 	]+comneps %xmm3,%xmm2,%xmm1
[ 	]+41b8:[ 	]+0f 25 2c 97 11 00 00 10 00 04[ 	]+comneps 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+41c2:[ 	]+0f 25 2c d3 10 00[ 	]+comeqps %xmm3,%xmm2,%xmm1
[ 	]+41c8:[ 	]+0f 25 2c d3 10 01[ 	]+comltps %xmm3,%xmm2,%xmm1
[ 	]+41ce:[ 	]+0f 25 2c d3 10 02[ 	]+comleps %xmm3,%xmm2,%xmm1
[ 	]+41d4:[ 	]+0f 25 2c d3 10 03[ 	]+comunordps %xmm3,%xmm2,%xmm1
[ 	]+41da:[ 	]+0f 25 2c d3 10 04[ 	]+comneps %xmm3,%xmm2,%xmm1
[ 	]+41e0:[ 	]+0f 25 2c d3 10 05[ 	]+comnltps %xmm3,%xmm2,%xmm1
[ 	]+41e6:[ 	]+0f 25 2c d3 10 06[ 	]+comnleps %xmm3,%xmm2,%xmm1
[ 	]+41ec:[ 	]+0f 25 2c d3 10 07[ 	]+comordps %xmm3,%xmm2,%xmm1
[ 	]+41f2:[ 	]+0f 25 2c d3 10 08[ 	]+comueqps %xmm3,%xmm2,%xmm1
[ 	]+41f8:[ 	]+0f 25 2c d3 10 09[ 	]+comultps %xmm3,%xmm2,%xmm1
[ 	]+41fe:[ 	]+0f 25 2c d3 10 0a[ 	]+comuleps %xmm3,%xmm2,%xmm1
[ 	]+4204:[ 	]+0f 25 2c d3 10 0b[ 	]+comfalseps %xmm3,%xmm2,%xmm1
[ 	]+420a:[ 	]+0f 25 2c d3 10 0c[ 	]+comuneps %xmm3,%xmm2,%xmm1
[ 	]+4210:[ 	]+0f 25 2c d3 10 0d[ 	]+comunltps %xmm3,%xmm2,%xmm1
[ 	]+4216:[ 	]+0f 25 2c d3 10 0e[ 	]+comunleps %xmm3,%xmm2,%xmm1
[ 	]+421c:[ 	]+0f 25 2c d3 10 0f[ 	]+comtrueps %xmm3,%xmm2,%xmm1
[ 	]+4222:[ 	]+0f 25 2c 97 11 00 00 10 00 00[ 	]+comeqps 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+422c:[ 	]+0f 25 2c 97 11 00 00 10 00 01[ 	]+comltps 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4236:[ 	]+0f 25 2c 97 11 00 00 10 00 02[ 	]+comleps 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4240:[ 	]+0f 25 2c 97 11 00 00 10 00 03[ 	]+comunordps 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+424a:[ 	]+0f 25 2c 97 11 00 00 10 00 04[ 	]+comneps 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4254:[ 	]+0f 25 2c 97 11 00 00 10 00 05[ 	]+comnltps 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+425e:[ 	]+0f 25 2c 97 11 00 00 10 00 06[ 	]+comnleps 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4268:[ 	]+0f 25 2c 97 11 00 00 10 00 07[ 	]+comordps 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4272:[ 	]+0f 25 2c 97 11 00 00 10 00 08[ 	]+comueqps 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+427c:[ 	]+0f 25 2c 97 11 00 00 10 00 09[ 	]+comultps 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4286:[ 	]+0f 25 2c 97 11 00 00 10 00 0a[ 	]+comuleps 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4290:[ 	]+0f 25 2c 97 11 00 00 10 00 0b[ 	]+comfalseps 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+429a:[ 	]+0f 25 2c 97 11 00 00 10 00 0c[ 	]+comuneps 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+42a4:[ 	]+0f 25 2c 97 11 00 00 10 00 0d[ 	]+comunltps 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+42ae:[ 	]+0f 25 2c 97 11 00 00 10 00 0e[ 	]+comunleps 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+42b8:[ 	]+0f 25 2c 97 11 00 00 10 00 0f[ 	]+comtrueps 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+42c2:[ 	]+0f 25 2d d3 10 04[ 	]+comnepd %xmm3,%xmm2,%xmm1
[ 	]+42c8:[ 	]+0f 25 2d 97 11 00 00 10 00 04[ 	]+comnepd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+42d2:[ 	]+0f 25 2d d3 10 00[ 	]+comeqpd %xmm3,%xmm2,%xmm1
[ 	]+42d8:[ 	]+0f 25 2d d3 10 01[ 	]+comltpd %xmm3,%xmm2,%xmm1
[ 	]+42de:[ 	]+0f 25 2d d3 10 02[ 	]+comlepd %xmm3,%xmm2,%xmm1
[ 	]+42e4:[ 	]+0f 25 2d d3 10 03[ 	]+comunordpd %xmm3,%xmm2,%xmm1
[ 	]+42ea:[ 	]+0f 25 2d d3 10 04[ 	]+comnepd %xmm3,%xmm2,%xmm1
[ 	]+42f0:[ 	]+0f 25 2d d3 10 05[ 	]+comnltpd %xmm3,%xmm2,%xmm1
[ 	]+42f6:[ 	]+0f 25 2d d3 10 06[ 	]+comnlepd %xmm3,%xmm2,%xmm1
[ 	]+42fc:[ 	]+0f 25 2d d3 10 07[ 	]+comordpd %xmm3,%xmm2,%xmm1
[ 	]+4302:[ 	]+0f 25 2d d3 10 08[ 	]+comueqpd %xmm3,%xmm2,%xmm1
[ 	]+4308:[ 	]+0f 25 2d d3 10 09[ 	]+comultpd %xmm3,%xmm2,%xmm1
[ 	]+430e:[ 	]+0f 25 2d d3 10 0a[ 	]+comulepd %xmm3,%xmm2,%xmm1
[ 	]+4314:[ 	]+0f 25 2d d3 10 0b[ 	]+comfalsepd %xmm3,%xmm2,%xmm1
[ 	]+431a:[ 	]+0f 25 2d d3 10 0c[ 	]+comunepd %xmm3,%xmm2,%xmm1
[ 	]+4320:[ 	]+0f 25 2d d3 10 0d[ 	]+comunltpd %xmm3,%xmm2,%xmm1
[ 	]+4326:[ 	]+0f 25 2d d3 10 0e[ 	]+comunlepd %xmm3,%xmm2,%xmm1
[ 	]+432c:[ 	]+0f 25 2d d3 10 0f[ 	]+comtruepd %xmm3,%xmm2,%xmm1
[ 	]+4332:[ 	]+0f 25 2d 97 11 00 00 10 00 00[ 	]+comeqpd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+433c:[ 	]+0f 25 2d 97 11 00 00 10 00 01[ 	]+comltpd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4346:[ 	]+0f 25 2d 97 11 00 00 10 00 02[ 	]+comlepd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4350:[ 	]+0f 25 2d 97 11 00 00 10 00 03[ 	]+comunordpd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+435a:[ 	]+0f 25 2d 97 11 00 00 10 00 04[ 	]+comnepd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4364:[ 	]+0f 25 2d 97 11 00 00 10 00 05[ 	]+comnltpd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+436e:[ 	]+0f 25 2d 97 11 00 00 10 00 06[ 	]+comnlepd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4378:[ 	]+0f 25 2d 97 11 00 00 10 00 07[ 	]+comordpd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4382:[ 	]+0f 25 2d 97 11 00 00 10 00 08[ 	]+comueqpd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+438c:[ 	]+0f 25 2d 97 11 00 00 10 00 09[ 	]+comultpd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4396:[ 	]+0f 25 2d 97 11 00 00 10 00 0a[ 	]+comulepd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+43a0:[ 	]+0f 25 2d 97 11 00 00 10 00 0b[ 	]+comfalsepd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+43aa:[ 	]+0f 25 2d 97 11 00 00 10 00 0c[ 	]+comunepd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+43b4:[ 	]+0f 25 2d 97 11 00 00 10 00 0d[ 	]+comunltpd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+43be:[ 	]+0f 25 2d 97 11 00 00 10 00 0e[ 	]+comunlepd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+43c8:[ 	]+0f 25 2d 97 11 00 00 10 00 0f[ 	]+comtruepd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+43d2:[ 	]+0f 25 4c d3 10 04[ 	]+pcomeqb %xmm3,%xmm2,%xmm1
[ 	]+43d8:[ 	]+0f 25 4c 97 11 00 00 10 00 04[ 	]+pcomeqb 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+43e2:[ 	]+0f 25 4c d3 10 00[ 	]+pcomltb %xmm3,%xmm2,%xmm1
[ 	]+43e8:[ 	]+0f 25 4c d3 10 01[ 	]+pcomleb %xmm3,%xmm2,%xmm1
[ 	]+43ee:[ 	]+0f 25 4c d3 10 02[ 	]+pcomgtb %xmm3,%xmm2,%xmm1
[ 	]+43f4:[ 	]+0f 25 4c d3 10 03[ 	]+pcomgeb %xmm3,%xmm2,%xmm1
[ 	]+43fa:[ 	]+0f 25 4c d3 10 04[ 	]+pcomeqb %xmm3,%xmm2,%xmm1
[ 	]+4400:[ 	]+0f 25 4c d3 10 05[ 	]+pcomneb %xmm3,%xmm2,%xmm1
[ 	]+4406:[ 	]+0f 25 4c 97 11 00 00 10 00 00[ 	]+pcomltb 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4410:[ 	]+0f 25 4c 97 11 00 00 10 00 01[ 	]+pcomleb 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+441a:[ 	]+0f 25 4c 97 11 00 00 10 00 02[ 	]+pcomgtb 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4424:[ 	]+0f 25 4c 97 11 00 00 10 00 03[ 	]+pcomgeb 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+442e:[ 	]+0f 25 4c 97 11 00 00 10 00 04[ 	]+pcomeqb 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4438:[ 	]+0f 25 4c 97 11 00 00 10 00 05[ 	]+pcomneb 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4442:[ 	]+0f 25 4d d3 10 04[ 	]+pcomeqw %xmm3,%xmm2,%xmm1
[ 	]+4448:[ 	]+0f 25 4d 97 11 00 00 10 00 04[ 	]+pcomeqw 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4452:[ 	]+0f 25 4d d3 10 00[ 	]+pcomltw %xmm3,%xmm2,%xmm1
[ 	]+4458:[ 	]+0f 25 4d d3 10 01[ 	]+pcomlew %xmm3,%xmm2,%xmm1
[ 	]+445e:[ 	]+0f 25 4d d3 10 02[ 	]+pcomgtw %xmm3,%xmm2,%xmm1
[ 	]+4464:[ 	]+0f 25 4d d3 10 03[ 	]+pcomgew %xmm3,%xmm2,%xmm1
[ 	]+446a:[ 	]+0f 25 4d d3 10 04[ 	]+pcomeqw %xmm3,%xmm2,%xmm1
[ 	]+4470:[ 	]+0f 25 4d d3 10 05[ 	]+pcomnew %xmm3,%xmm2,%xmm1
[ 	]+4476:[ 	]+0f 25 4d 97 11 00 00 10 00 00[ 	]+pcomltw 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4480:[ 	]+0f 25 4d 97 11 00 00 10 00 01[ 	]+pcomlew 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+448a:[ 	]+0f 25 4d 97 11 00 00 10 00 02[ 	]+pcomgtw 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4494:[ 	]+0f 25 4d 97 11 00 00 10 00 03[ 	]+pcomgew 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+449e:[ 	]+0f 25 4d 97 11 00 00 10 00 04[ 	]+pcomeqw 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+44a8:[ 	]+0f 25 4d 97 11 00 00 10 00 05[ 	]+pcomnew 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+44b2:[ 	]+0f 25 4e d3 10 04[ 	]+pcomeqd %xmm3,%xmm2,%xmm1
[ 	]+44b8:[ 	]+0f 25 4e 97 11 00 00 10 00 04[ 	]+pcomeqd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+44c2:[ 	]+0f 25 4e d3 10 00[ 	]+pcomltd %xmm3,%xmm2,%xmm1
[ 	]+44c8:[ 	]+0f 25 4e d3 10 01[ 	]+pcomled %xmm3,%xmm2,%xmm1
[ 	]+44ce:[ 	]+0f 25 4e d3 10 02[ 	]+pcomgtd %xmm3,%xmm2,%xmm1
[ 	]+44d4:[ 	]+0f 25 4e d3 10 03[ 	]+pcomged %xmm3,%xmm2,%xmm1
[ 	]+44da:[ 	]+0f 25 4e d3 10 04[ 	]+pcomeqd %xmm3,%xmm2,%xmm1
[ 	]+44e0:[ 	]+0f 25 4e d3 10 05[ 	]+pcomned %xmm3,%xmm2,%xmm1
[ 	]+44e6:[ 	]+0f 25 4e 97 11 00 00 10 00 00[ 	]+pcomltd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+44f0:[ 	]+0f 25 4e 97 11 00 00 10 00 01[ 	]+pcomled 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+44fa:[ 	]+0f 25 4e 97 11 00 00 10 00 02[ 	]+pcomgtd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4504:[ 	]+0f 25 4e 97 11 00 00 10 00 03[ 	]+pcomged 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+450e:[ 	]+0f 25 4e 97 11 00 00 10 00 04[ 	]+pcomeqd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4518:[ 	]+0f 25 4e 97 11 00 00 10 00 05[ 	]+pcomned 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4522:[ 	]+0f 25 4f d3 10 04[ 	]+pcomeqq %xmm3,%xmm2,%xmm1
[ 	]+4528:[ 	]+0f 25 4f 97 11 00 00 10 00 04[ 	]+pcomeqq 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4532:[ 	]+0f 25 4f d3 10 00[ 	]+pcomltq %xmm3,%xmm2,%xmm1
[ 	]+4538:[ 	]+0f 25 4f d3 10 01[ 	]+pcomleq %xmm3,%xmm2,%xmm1
[ 	]+453e:[ 	]+0f 25 4f d3 10 02[ 	]+pcomgtq %xmm3,%xmm2,%xmm1
[ 	]+4544:[ 	]+0f 25 4f d3 10 03[ 	]+pcomgeq %xmm3,%xmm2,%xmm1
[ 	]+454a:[ 	]+0f 25 4f d3 10 04[ 	]+pcomeqq %xmm3,%xmm2,%xmm1
[ 	]+4550:[ 	]+0f 25 4f d3 10 05[ 	]+pcomneq %xmm3,%xmm2,%xmm1
[ 	]+4556:[ 	]+0f 25 4f 97 11 00 00 10 00 00[ 	]+pcomltq 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4560:[ 	]+0f 25 4f 97 11 00 00 10 00 01[ 	]+pcomleq 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+456a:[ 	]+0f 25 4f 97 11 00 00 10 00 02[ 	]+pcomgtq 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4574:[ 	]+0f 25 4f 97 11 00 00 10 00 03[ 	]+pcomgeq 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+457e:[ 	]+0f 25 4f 97 11 00 00 10 00 04[ 	]+pcomeqq 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4588:[ 	]+0f 25 4f 97 11 00 00 10 00 05[ 	]+pcomneq 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4592:[ 	]+0f 25 6c d3 10 04[ 	]+pcomequb %xmm3,%xmm2,%xmm1
[ 	]+4598:[ 	]+0f 25 6c 97 11 00 00 10 00 04[ 	]+pcomequb 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+45a2:[ 	]+0f 25 6c d3 10 00[ 	]+pcomltub %xmm3,%xmm2,%xmm1
[ 	]+45a8:[ 	]+0f 25 6c d3 10 01[ 	]+pcomleub %xmm3,%xmm2,%xmm1
[ 	]+45ae:[ 	]+0f 25 6c d3 10 02[ 	]+pcomgtub %xmm3,%xmm2,%xmm1
[ 	]+45b4:[ 	]+0f 25 6c d3 10 03[ 	]+pcomgeub %xmm3,%xmm2,%xmm1
[ 	]+45ba:[ 	]+0f 25 6c d3 10 04[ 	]+pcomequb %xmm3,%xmm2,%xmm1
[ 	]+45c0:[ 	]+0f 25 6c d3 10 05[ 	]+pcomneub %xmm3,%xmm2,%xmm1
[ 	]+45c6:[ 	]+0f 25 6c 97 11 00 00 10 00 00[ 	]+pcomltub 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+45d0:[ 	]+0f 25 6c 97 11 00 00 10 00 01[ 	]+pcomleub 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+45da:[ 	]+0f 25 6c 97 11 00 00 10 00 02[ 	]+pcomgtub 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+45e4:[ 	]+0f 25 6c 97 11 00 00 10 00 03[ 	]+pcomgeub 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+45ee:[ 	]+0f 25 6c 97 11 00 00 10 00 04[ 	]+pcomequb 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+45f8:[ 	]+0f 25 6c 97 11 00 00 10 00 05[ 	]+pcomneub 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4602:[ 	]+0f 25 6d d3 10 04[ 	]+pcomequw %xmm3,%xmm2,%xmm1
[ 	]+4608:[ 	]+0f 25 6d 97 11 00 00 10 00 04[ 	]+pcomequw 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4612:[ 	]+0f 25 6d d3 10 00[ 	]+pcomltuw %xmm3,%xmm2,%xmm1
[ 	]+4618:[ 	]+0f 25 6d d3 10 01[ 	]+pcomleuw %xmm3,%xmm2,%xmm1
[ 	]+461e:[ 	]+0f 25 6d d3 10 02[ 	]+pcomgtuw %xmm3,%xmm2,%xmm1
[ 	]+4624:[ 	]+0f 25 6d d3 10 03[ 	]+pcomgeuw %xmm3,%xmm2,%xmm1
[ 	]+462a:[ 	]+0f 25 6d d3 10 04[ 	]+pcomequw %xmm3,%xmm2,%xmm1
[ 	]+4630:[ 	]+0f 25 6d d3 10 05[ 	]+pcomneuw %xmm3,%xmm2,%xmm1
[ 	]+4636:[ 	]+0f 25 6d 97 11 00 00 10 00 00[ 	]+pcomltuw 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4640:[ 	]+0f 25 6d 97 11 00 00 10 00 01[ 	]+pcomleuw 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+464a:[ 	]+0f 25 6d 97 11 00 00 10 00 02[ 	]+pcomgtuw 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4654:[ 	]+0f 25 6d 97 11 00 00 10 00 03[ 	]+pcomgeuw 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+465e:[ 	]+0f 25 6d 97 11 00 00 10 00 04[ 	]+pcomequw 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4668:[ 	]+0f 25 6d 97 11 00 00 10 00 05[ 	]+pcomneuw 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4672:[ 	]+0f 25 6e d3 10 04[ 	]+pcomequd %xmm3,%xmm2,%xmm1
[ 	]+4678:[ 	]+0f 25 6e 97 11 00 00 10 00 04[ 	]+pcomequd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4682:[ 	]+0f 25 6e d3 10 00[ 	]+pcomltud %xmm3,%xmm2,%xmm1
[ 	]+4688:[ 	]+0f 25 6e d3 10 01[ 	]+pcomleud %xmm3,%xmm2,%xmm1
[ 	]+468e:[ 	]+0f 25 6e d3 10 02[ 	]+pcomgtud %xmm3,%xmm2,%xmm1
[ 	]+4694:[ 	]+0f 25 6e d3 10 03[ 	]+pcomgeud %xmm3,%xmm2,%xmm1
[ 	]+469a:[ 	]+0f 25 6e d3 10 04[ 	]+pcomequd %xmm3,%xmm2,%xmm1
[ 	]+46a0:[ 	]+0f 25 6e d3 10 05[ 	]+pcomneud %xmm3,%xmm2,%xmm1
[ 	]+46a6:[ 	]+0f 25 6e 97 11 00 00 10 00 00[ 	]+pcomltud 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+46b0:[ 	]+0f 25 6e 97 11 00 00 10 00 01[ 	]+pcomleud 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+46ba:[ 	]+0f 25 6e 97 11 00 00 10 00 02[ 	]+pcomgtud 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+46c4:[ 	]+0f 25 6e 97 11 00 00 10 00 03[ 	]+pcomgeud 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+46ce:[ 	]+0f 25 6e 97 11 00 00 10 00 04[ 	]+pcomequd 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+46d8:[ 	]+0f 25 6e 97 11 00 00 10 00 05[ 	]+pcomneud 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+46e2:[ 	]+0f 25 6f d3 10 04[ 	]+pcomequq %xmm3,%xmm2,%xmm1
[ 	]+46e8:[ 	]+0f 25 6f 97 11 00 00 10 00 04[ 	]+pcomequq 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+46f2:[ 	]+0f 25 6f d3 10 00[ 	]+pcomltuq %xmm3,%xmm2,%xmm1
[ 	]+46f8:[ 	]+0f 25 6f d3 10 01[ 	]+pcomleuq %xmm3,%xmm2,%xmm1
[ 	]+46fe:[ 	]+0f 25 6f d3 10 02[ 	]+pcomgtuq %xmm3,%xmm2,%xmm1
[ 	]+4704:[ 	]+0f 25 6f d3 10 03[ 	]+pcomgeuq %xmm3,%xmm2,%xmm1
[ 	]+470a:[ 	]+0f 25 6f d3 10 04[ 	]+pcomequq %xmm3,%xmm2,%xmm1
[ 	]+4710:[ 	]+0f 25 6f d3 10 05[ 	]+pcomneuq %xmm3,%xmm2,%xmm1
[ 	]+4716:[ 	]+0f 25 6f 97 11 00 00 10 00 00[ 	]+pcomltuq 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4720:[ 	]+0f 25 6f 97 11 00 00 10 00 01[ 	]+pcomleuq 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+472a:[ 	]+0f 25 6f 97 11 00 00 10 00 02[ 	]+pcomgtuq 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4734:[ 	]+0f 25 6f 97 11 00 00 10 00 03[ 	]+pcomgeuq 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+473e:[ 	]+0f 25 6f 97 11 00 00 10 00 04[ 	]+pcomequq 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4748:[ 	]+0f 25 6f 97 11 00 00 10 00 05[ 	]+pcomneuq 0x100000\(%r15\),%xmm2,%xmm1
[ 	]+4752:[ 	]+0f 7a 12 ca[ 	]+frczss %xmm2,%xmm1
[ 	]+4756:[ 	]+0f 7a 12 4a 04[ 	]+frczss 0x4\(%rdx\),%xmm1
[ 	]+475b:[ 	]+0f 7a 13 ca[ 	]+frczsd %xmm2,%xmm1
[ 	]+475f:[ 	]+0f 7a 13 4a 04[ 	]+frczsd 0x4\(%rdx\),%xmm1
[ 	]+4764:[ 	]+0f 7a 10 ca[ 	]+frczps %xmm2,%xmm1
[ 	]+4768:[ 	]+0f 7a 10 4a 04[ 	]+frczps 0x4\(%rdx\),%xmm1
[ 	]+476d:[ 	]+0f 7a 11 ca[ 	]+frczpd %xmm2,%xmm1
[ 	]+4771:[ 	]+0f 7a 11 4a 04[ 	]+frczpd 0x4\(%rdx\),%xmm1
[ 	]+4776:[ 	]+45 0f 7a 12 dc[ 	]+frczss %xmm12,%xmm11
[ 	]+477b:[ 	]+45 0f 7a 12 9f 00 00 10 00[ 	]+frczss 0x100000\(%r15\),%xmm11
[ 	]+4784:[ 	]+45 0f 7a 13 dc[ 	]+frczsd %xmm12,%xmm11
[ 	]+4789:[ 	]+45 0f 7a 13 9f 00 00 10 00[ 	]+frczsd 0x100000\(%r15\),%xmm11
[ 	]+4792:[ 	]+45 0f 7a 10 dc[ 	]+frczps %xmm12,%xmm11
[ 	]+4797:[ 	]+45 0f 7a 10 9f 00 00 10 00[ 	]+frczps 0x100000\(%r15\),%xmm11
[ 	]+47a0:[ 	]+45 0f 7a 11 dc[ 	]+frczpd %xmm12,%xmm11
[ 	]+47a5:[ 	]+45 0f 7a 11 9f 00 00 10 00[ 	]+frczpd 0x100000\(%r15\),%xmm11
[ 	]+47ae:[ 	]+41 0f 7a 12 cc[ 	]+frczss %xmm12,%xmm1
[ 	]+47b3:[ 	]+0f 7a 12 4a 04[ 	]+frczss 0x4\(%rdx\),%xmm1
[ 	]+47b8:[ 	]+41 0f 7a 13 cc[ 	]+frczsd %xmm12,%xmm1
[ 	]+47bd:[ 	]+0f 7a 13 4a 04[ 	]+frczsd 0x4\(%rdx\),%xmm1
[ 	]+47c2:[ 	]+41 0f 7a 10 cc[ 	]+frczps %xmm12,%xmm1
[ 	]+47c7:[ 	]+0f 7a 10 4a 04[ 	]+frczps 0x4\(%rdx\),%xmm1
[ 	]+47cc:[ 	]+41 0f 7a 11 cc[ 	]+frczpd %xmm12,%xmm1
[ 	]+47d1:[ 	]+0f 7a 11 4a 04[ 	]+frczpd 0x4\(%rdx\),%xmm1
[ 	]+47d6:[ 	]+44 0f 7a 12 da[ 	]+frczss %xmm2,%xmm11
[ 	]+47db:[ 	]+44 0f 7a 12 5a 04[ 	]+frczss 0x4\(%rdx\),%xmm11
[ 	]+47e1:[ 	]+44 0f 7a 13 da[ 	]+frczsd %xmm2,%xmm11
[ 	]+47e6:[ 	]+44 0f 7a 13 5a 04[ 	]+frczsd 0x4\(%rdx\),%xmm11
[ 	]+47ec:[ 	]+44 0f 7a 10 da[ 	]+frczps %xmm2,%xmm11
[ 	]+47f1:[ 	]+44 0f 7a 10 5a 04[ 	]+frczps 0x4\(%rdx\),%xmm11
[ 	]+47f7:[ 	]+44 0f 7a 11 da[ 	]+frczpd %xmm2,%xmm11
[ 	]+47fc:[ 	]+44 0f 7a 11 5a 04[ 	]+frczpd 0x4\(%rdx\),%xmm11
[ 	]+4802:[ 	]+0f 7a 12 ca[ 	]+frczss %xmm2,%xmm1
[ 	]+4806:[ 	]+0f 7a 12 4a 04[ 	]+frczss 0x4\(%rdx\),%xmm1
[ 	]+480b:[ 	]+0f 7a 13 ca[ 	]+frczsd %xmm2,%xmm1
[ 	]+480f:[ 	]+0f 7a 13 4a 04[ 	]+frczsd 0x4\(%rdx\),%xmm1
[ 	]+4814:[ 	]+0f 7a 10 ca[ 	]+frczps %xmm2,%xmm1
[ 	]+4818:[ 	]+0f 7a 10 4a 04[ 	]+frczps 0x4\(%rdx\),%xmm1
[ 	]+481d:[ 	]+0f 7a 11 ca[ 	]+frczpd %xmm2,%xmm1
[ 	]+4821:[ 	]+0f 7a 11 4a 04[ 	]+frczpd 0x4\(%rdx\),%xmm1
[ 	]+4826:[ 	]+0f 7a 12 ca[ 	]+frczss %xmm2,%xmm1
[ 	]+482a:[ 	]+41 0f 7a 12 8f 00 00 10 00[ 	]+frczss 0x100000\(%r15\),%xmm1
[ 	]+4833:[ 	]+0f 7a 13 ca[ 	]+frczsd %xmm2,%xmm1
[ 	]+4837:[ 	]+41 0f 7a 13 8f 00 00 10 00[ 	]+frczsd 0x100000\(%r15\),%xmm1
[ 	]+4840:[ 	]+0f 7a 10 ca[ 	]+frczps %xmm2,%xmm1
[ 	]+4844:[ 	]+41 0f 7a 10 8f 00 00 10 00[ 	]+frczps 0x100000\(%r15\),%xmm1
[ 	]+484d:[ 	]+0f 7a 11 ca[ 	]+frczpd %xmm2,%xmm1
[ 	]+4851:[ 	]+41 0f 7a 11 8f 00 00 10 00[ 	]+frczpd 0x100000\(%r15\),%xmm1
[ 	]+485a:[ 	]+0f 7a 30 ca[ 	]+cvtph2ps %xmm2,%xmm1
[ 	]+485e:[ 	]+0f 7a 30 4a 04[ 	]+cvtph2ps 0x4\(%rdx\),%xmm1
[ 	]+4863:[ 	]+0f 7a 31 d1[ 	]+cvtps2ph %xmm2,%xmm1
[ 	]+4867:[ 	]+0f 7a 31 4a 04[ 	]+cvtps2ph %xmm1,0x4\(%rdx\)
[ 	]+486c:[ 	]+45 0f 7a 30 dc[ 	]+cvtph2ps %xmm12,%xmm11
[ 	]+4871:[ 	]+45 0f 7a 30 9f 00 00 10 00[ 	]+cvtph2ps 0x100000\(%r15\),%xmm11
[ 	]+487a:[ 	]+45 0f 7a 31 e3[ 	]+cvtps2ph %xmm12,%xmm11
[ 	]+487f:[ 	]+45 0f 7a 31 9f 00 00 10 00[ 	]+cvtps2ph %xmm11,0x100000\(%r15\)
[ 	]+4888:[ 	]+41 0f 7a 30 cc[ 	]+cvtph2ps %xmm12,%xmm1
[ 	]+488d:[ 	]+0f 7a 30 4a 04[ 	]+cvtph2ps 0x4\(%rdx\),%xmm1
[ 	]+4892:[ 	]+44 0f 7a 31 e1[ 	]+cvtps2ph %xmm12,%xmm1
[ 	]+4897:[ 	]+0f 7a 31 4a 04[ 	]+cvtps2ph %xmm1,0x4\(%rdx\)
[ 	]+489c:[ 	]+44 0f 7a 30 da[ 	]+cvtph2ps %xmm2,%xmm11
[ 	]+48a1:[ 	]+44 0f 7a 30 5a 04[ 	]+cvtph2ps 0x4\(%rdx\),%xmm11
[ 	]+48a7:[ 	]+41 0f 7a 31 d3[ 	]+cvtps2ph %xmm2,%xmm11
[ 	]+48ac:[ 	]+44 0f 7a 31 5a 04[ 	]+cvtps2ph %xmm11,0x4\(%rdx\)
[ 	]+48b2:[ 	]+0f 7a 30 ca[ 	]+cvtph2ps %xmm2,%xmm1
[ 	]+48b6:[ 	]+0f 7a 30 4a 04[ 	]+cvtph2ps 0x4\(%rdx\),%xmm1
[ 	]+48bb:[ 	]+0f 7a 31 d1[ 	]+cvtps2ph %xmm2,%xmm1
[ 	]+48bf:[ 	]+0f 7a 31 4a 04[ 	]+cvtps2ph %xmm1,0x4\(%rdx\)
[ 	]+48c4:[ 	]+0f 7a 30 ca[ 	]+cvtph2ps %xmm2,%xmm1
[ 	]+48c8:[ 	]+41 0f 7a 30 8f 00 00 10 00[ 	]+cvtph2ps 0x100000\(%r15\),%xmm1
[ 	]+48d1:[ 	]+0f 7a 31 d1[ 	]+cvtps2ph %xmm2,%xmm1
[ 	]+48d5:[ 	]+41 0f 7a 31 8f 00 00 10 00[ 	]+cvtps2ph %xmm1,0x100000\(%r15\)
[ 	]+48de:[ 	]+0f 7b 40 41 04 04[ 	]+protb[ 	]+\$0x4,0x4\(%rcx\),%xmm0
[ 	]+48e4:[ 	]+41 0f 7b 41 42 08 01[ 	]+protw[ 	]+\$0x1,0x8\(%r10\),%xmm0
[ 	]+48eb:[ 	]+41 0f 7b 43 41 04 03[ 	]+protq[ 	]+\$0x3,0x4\(%r9\),%xmm0
[ 	]+48f2:[ 	]+c3[ 	]+retq[ 	]*
