#as:
#objdump: -dwMintel -Msuffix
#name: i386 AVX512F opts insns (Intel disassembly)
#source: avx512f-opts.s

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
[ 	]*[a-f0-9]+:	62 f1 fd 48 29 ee    	vmovapd.s zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 48 28 f5    	vmovapd zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 4f 29 ee    	vmovapd.s zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 4f 28 f5    	vmovapd zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd cf 29 ee    	vmovapd.s zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd cf 28 f5    	vmovapd zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7c 48 29 ee    	vmovaps.s zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 7c 48 28 f5    	vmovaps zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 7c 4f 29 ee    	vmovaps.s zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7c 4f 28 f5    	vmovaps zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7c cf 29 ee    	vmovaps.s zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7c cf 28 f5    	vmovaps zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7d 48 7f ee    	vmovdqa32.s zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 7d 48 6f f5    	vmovdqa32 zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 7d 4f 7f ee    	vmovdqa32.s zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7d 4f 6f f5    	vmovdqa32 zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7d cf 7f ee    	vmovdqa32.s zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7d cf 6f f5    	vmovdqa32 zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 48 7f ee    	vmovdqa64.s zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 48 6f f5    	vmovdqa64 zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 4f 7f ee    	vmovdqa64.s zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 4f 6f f5    	vmovdqa64 zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd cf 7f ee    	vmovdqa64.s zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd cf 6f f5    	vmovdqa64 zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7e 48 7f ee    	vmovdqu32.s zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 7e 48 6f f5    	vmovdqu32 zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 7e 4f 7f ee    	vmovdqu32.s zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7e 4f 6f f5    	vmovdqu32 zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7e cf 7f ee    	vmovdqu32.s zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7e cf 6f f5    	vmovdqu32 zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fe 48 7f ee    	vmovdqu64.s zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 fe 48 6f f5    	vmovdqu64 zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 fe 4f 7f ee    	vmovdqu64.s zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fe 4f 6f f5    	vmovdqu64 zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fe cf 7f ee    	vmovdqu64.s zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fe cf 6f f5    	vmovdqu64 zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 d7 0f 11 e6    	vmovsd.s xmm6\{k7\},xmm5,xmm4
[ 	]*[a-f0-9]+:	62 f1 d7 0f 10 f4    	vmovsd xmm6\{k7\},xmm5,xmm4
[ 	]*[a-f0-9]+:	62 f1 d7 8f 11 e6    	vmovsd.s xmm6\{k7\}\{z\},xmm5,xmm4
[ 	]*[a-f0-9]+:	62 f1 d7 8f 10 f4    	vmovsd xmm6\{k7\}\{z\},xmm5,xmm4
[ 	]*[a-f0-9]+:	62 f1 56 0f 11 e6    	vmovss.s xmm6\{k7\},xmm5,xmm4
[ 	]*[a-f0-9]+:	62 f1 56 0f 10 f4    	vmovss xmm6\{k7\},xmm5,xmm4
[ 	]*[a-f0-9]+:	62 f1 56 8f 11 e6    	vmovss.s xmm6\{k7\}\{z\},xmm5,xmm4
[ 	]*[a-f0-9]+:	62 f1 56 8f 10 f4    	vmovss xmm6\{k7\}\{z\},xmm5,xmm4
[ 	]*[a-f0-9]+:	62 f1 fd 48 11 ee    	vmovupd.s zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 48 10 f5    	vmovupd zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 4f 11 ee    	vmovupd.s zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 4f 10 f5    	vmovupd zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd cf 11 ee    	vmovupd.s zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd cf 10 f5    	vmovupd zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7c 48 11 ee    	vmovups.s zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 7c 48 10 f5    	vmovups zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 7c 4f 11 ee    	vmovups.s zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7c 4f 10 f5    	vmovups zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7c cf 11 ee    	vmovups.s zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7c cf 10 f5    	vmovups zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 48 29 ee    	vmovapd.s zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 48 28 f5    	vmovapd zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 4f 29 ee    	vmovapd.s zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 4f 28 f5    	vmovapd zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd cf 29 ee    	vmovapd.s zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd cf 28 f5    	vmovapd zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7c 48 29 ee    	vmovaps.s zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 7c 48 28 f5    	vmovaps zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 7c 4f 29 ee    	vmovaps.s zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7c 4f 28 f5    	vmovaps zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7c cf 29 ee    	vmovaps.s zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7c cf 28 f5    	vmovaps zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7d 48 7f ee    	vmovdqa32.s zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 7d 48 6f f5    	vmovdqa32 zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 7d 4f 7f ee    	vmovdqa32.s zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7d 4f 6f f5    	vmovdqa32 zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7d cf 7f ee    	vmovdqa32.s zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7d cf 6f f5    	vmovdqa32 zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 48 7f ee    	vmovdqa64.s zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 48 6f f5    	vmovdqa64 zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 4f 7f ee    	vmovdqa64.s zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 4f 6f f5    	vmovdqa64 zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd cf 7f ee    	vmovdqa64.s zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd cf 6f f5    	vmovdqa64 zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7e 48 7f ee    	vmovdqu32.s zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 7e 48 6f f5    	vmovdqu32 zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 7e 4f 7f ee    	vmovdqu32.s zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7e 4f 6f f5    	vmovdqu32 zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7e cf 7f ee    	vmovdqu32.s zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7e cf 6f f5    	vmovdqu32 zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fe 48 7f ee    	vmovdqu64.s zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 fe 48 6f f5    	vmovdqu64 zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 fe 4f 7f ee    	vmovdqu64.s zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fe 4f 6f f5    	vmovdqu64 zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fe cf 7f ee    	vmovdqu64.s zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fe cf 6f f5    	vmovdqu64 zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 d7 0f 11 e6    	vmovsd.s xmm6\{k7\},xmm5,xmm4
[ 	]*[a-f0-9]+:	62 f1 d7 0f 10 f4    	vmovsd xmm6\{k7\},xmm5,xmm4
[ 	]*[a-f0-9]+:	62 f1 d7 8f 11 e6    	vmovsd.s xmm6\{k7\}\{z\},xmm5,xmm4
[ 	]*[a-f0-9]+:	62 f1 d7 8f 10 f4    	vmovsd xmm6\{k7\}\{z\},xmm5,xmm4
[ 	]*[a-f0-9]+:	62 f1 56 0f 11 e6    	vmovss.s xmm6\{k7\},xmm5,xmm4
[ 	]*[a-f0-9]+:	62 f1 56 0f 10 f4    	vmovss xmm6\{k7\},xmm5,xmm4
[ 	]*[a-f0-9]+:	62 f1 56 8f 11 e6    	vmovss.s xmm6\{k7\}\{z\},xmm5,xmm4
[ 	]*[a-f0-9]+:	62 f1 56 8f 10 f4    	vmovss xmm6\{k7\}\{z\},xmm5,xmm4
[ 	]*[a-f0-9]+:	62 f1 fd 48 11 ee    	vmovupd.s zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 48 10 f5    	vmovupd zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 4f 11 ee    	vmovupd.s zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd 4f 10 f5    	vmovupd zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd cf 11 ee    	vmovupd.s zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 fd cf 10 f5    	vmovupd zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7c 48 11 ee    	vmovups.s zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 7c 48 10 f5    	vmovups zmm6,zmm5
[ 	]*[a-f0-9]+:	62 f1 7c 4f 11 ee    	vmovups.s zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7c 4f 10 f5    	vmovups zmm6\{k7\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7c cf 11 ee    	vmovups.s zmm6\{k7\}\{z\},zmm5
[ 	]*[a-f0-9]+:	62 f1 7c cf 10 f5    	vmovups zmm6\{k7\}\{z\},zmm5
#pass
