.*: Assembler messages:
.*:4: Error: .*
.*:5: Error: .*
.*:6: Error: .*
.*:7: Error: .*
.*:8: Error: .*
.*:9: Error: .*
.*:10: Error: .*
.*:11: Error: .*
.*:12: Error: .*
.*:13: Error: .*
.*:14: Error: .*
.*:15: Error: .*
.*:16: Error: .*
.*:17: Error: .*
.*:18: Error: .*
.*:19: Error: .*
.*:20: Error: .*
.*:21: Error: .*
.*:22: Error: .*
.*:23: Error: .*
.*:24: Error: .*
.*:25: Error: .*
.*:26: Error: .*
.*:27: Error: .*
.*:28: Error: .*
.*:31: Error: .*
.*:32: Error: .*
.*:33: Error: .*
.*:34: Error: .*
.*:35: Error: .*
.*:36: Error: .*
.*:37: Error: .*
.*:38: Error: .*
.*:39: Error: .*
.*:40: Error: .*
.*:41: Error: .*
.*:42: Error: .*
.*:43: Error: .*
.*:44: Error: .*
.*:45: Error: .*
.*:46: Error: .*
.*:47: Error: .*
.*:48: Error: .*
.*:49: Error: .*
.*:50: Error: .*
.*:51: Error: .*
.*:52: Error: .*
.*:53: Error: .*
.*:54: Error: .*
.*:55: Error: .*
GAS LISTING .*


[ 	]*1[ 	]+\# Check illegal 64bit AVX instructions
[ 	]*2[ 	]+\.text
[ 	]*3[ 	]+_start:
[ 	]*4[ 	]+vcvtpd2dq \(%rcx\),%xmm2
[ 	]*5[ 	]+vcvtpd2ps \(%rcx\),%xmm2
[ 	]*6[ 	]+vcvttpd2dq \(%rcx\),%xmm2
[ 	]*7[ 	]+vfmaddpd \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*8[ 	]+vfmaddps \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*9[ 	]+vfmaddsd \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*10[ 	]+vfmaddss \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*11[ 	]+vfmaddsubpd \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*12[ 	]+vfmaddsubps \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*13[ 	]+vfmsubaddpd \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*14[ 	]+vfmsubaddps \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*15[ 	]+vfmsubpd \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*16[ 	]+vfmsubps \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*17[ 	]+vfmsubsd \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*18[ 	]+vfmsubss \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*19[ 	]+vfnmaddpd \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*20[ 	]+vfnmaddps \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*21[ 	]+vfnmaddsd \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*22[ 	]+vfnmaddss \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*23[ 	]+vfnmsubpd \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*24[ 	]+vfnmsubps \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*25[ 	]+vfnmsubsd \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*26[ 	]+vfnmsubss \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*27[ 	]+vpermil2pd \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*28[ 	]+vpermil2ps \$17,%xmm4,%xmm2,%xmm1,%xmm3
[ 	]*29[ 	]+
[ 	]*30[ 	]+\.intel_syntax noprefix
[ 	]*31[ 	]+vcvtpd2dq xmm2,\[rcx\]
[ 	]*32[ 	]+vcvtpd2ps xmm2,\[rcx\]
[ 	]*33[ 	]+vcvttpd2dq xmm2,\[rcx\]
[ 	]*34[ 	]+vfmaddpd xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*35[ 	]+vfmaddps xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*36[ 	]+vfmaddsd xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*37[ 	]+vfmaddss xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*38[ 	]+vfmaddsubpd xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*39[ 	]+vfmaddsubps xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*40[ 	]+vfmsubaddpd xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*41[ 	]+vfmsubaddps xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*42[ 	]+vfmsubpd xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*43[ 	]+vfmsubps xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*44[ 	]+vfmsubsd xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*45[ 	]+vfmsubss xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*46[ 	]+vfnmaddpd xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*47[ 	]+vfnmaddps xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*48[ 	]+vfnmaddsd xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*49[ 	]+vfnmaddss xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*50[ 	]+vfnmsubpd xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*51[ 	]+vfnmsubps xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*52[ 	]+vfnmsubsd xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*53[ 	]+vfnmsubss xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*54[ 	]+vpermil2pd xmm3,xmm1,xmm2,xmm4,0x10
[ 	]*55[ 	]+vpermil2ps xmm3,xmm1,xmm2,xmm4,0x10
