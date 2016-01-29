.*: Assembler messages:
.*:5: Error: .*
.*:6: Error: .*
.*:7: Error: .*
.*:8: Error: .*
.*:9: Error: .*
.*:10: Error: .*
.*:12: Error: .*
.*:13: Error: .*
.*:15: Error: .*
.*:16: Error: .*
.*:18: Error: .*
.*:19: Error: .*
.*:20: Error: .*
.*:21: Error: .*
.*:22: Error: .*
.*:25: Error: .*
.*:26: Error: .*
.*:27: Error: .*
.*:28: Error: .*
.*:28: Error: .*
.*:29: Error: .*
.*:29: Error: .*
.*:30: Error: .*
.*:30: Error: .*
.*:32: Error: .*
.*:33: Error: .*
.*:35: Error: .*
.*:36: Error: .*
.*:38: Error: .*
.*:39: Error: .*
.*:40: Error: .*
.*:41: Error: .*
.*:42: Error: .*
.*:44: Error: .*
.*:45: Error: .*
.*:46: Error: .*
.*:47: Error: .*
.*:48: Error: .*
.*:49: Error: .*
.*:50: Error: .*
GAS LISTING .*


[ 	]*1[ 	]+# Check illegal AVX512F instructions
[ 	]*2[ 	]+\.text
[ 	]*3[ 	]+\.allow_index_reg
[ 	]*4[ 	]+_start:
[ 	]*5[ 	]+mov \{sae\}, %eax\{%k1\}
[ 	]*6[ 	]+mov \{sae\}, %eax
[ 	]*7[ 	]+mov %ebx, %eax\{%k2\}
[ 	]*8[ 	]+vaddps %zmm3, %zmm1, %zmm2\{z\}\{%k1\}\{z\}
[ 	]*9[ 	]+vaddps %zmm3, %zmm1\{%k3\}, %zmm2\{z\}
[ 	]*10[ 	]+vaddps %zmm3, %zmm1\{%k1\}, %zmm2\{%k2\}
[ 	]*11[ 	]+
[ 	]*12[ 	]+vcvtps2pd \(%eax\), %zmm1\{1to8\}
[ 	]*13[ 	]+vcvtps2pd \(%eax\)\{1to16\}, %zmm1
[ 	]*14[ 	]+
[ 	]*15[ 	]+vcvtps2pd \(%eax\)\{%k1\}, %zmm1
[ 	]*16[ 	]+vcvtps2pd \(%eax\)\{z\}, %zmm1
[ 	]*17[ 	]+
[ 	]*18[ 	]+vgatherqpd \(%rdi,%zmm2,8\),%zmm6
[ 	]*19[ 	]+vgatherqpd \(%edi\),%zmm6\{%k1\}
[ 	]*20[ 	]+vgatherqpd \(%zmm2\),%zmm6\{%k1\}
[ 	]*21[ 	]+vpscatterdd %zmm6,\(%edi\)\{%k1\}
[ 	]*22[ 	]+vpscatterdd %zmm6,\(%zmm2\)\{%k1\}
[ 	]*23[ 	]+
[ 	]*24[ 	]+\.intel_syntax noprefix
[ 	]*25[ 	]+mov eax\{k1\}, \{sae\}
[ 	]*26[ 	]+mov eax, \{sae\}
[ 	]*27[ 	]+mov eax\{k2\}, ebx
[ 	]*28[ 	]+vaddps zmm2\{z\}\{k1\}\{z\}, zmm1, zmm3
[ 	]*29[ 	]+vaddps zmm2\{z\}, zmm1\{k3\}, zmm3
[ 	]*30[ 	]+vaddps zmm2\{k2\}, zmm1\{k1\}, zmm3
[ 	]*31[ 	]+
[ 	]*32[ 	]+vcvtps2pd zmm1\{1to8\}, \[eax\]
[ 	]*33[ 	]+vcvtps2pd zmm1, \[eax\]\{1to16\}
[ 	]*34[ 	]+
[ 	]*35[ 	]+vcvtps2pd zmm1, \[eax\]\{k1\}
[ 	]*36[ 	]+vcvtps2pd zmm1, \[eax\]\{z\}
[ 	]*37[ 	]+
[ 	]*38[ 	]+vgatherqpd zmm6, ZMMWORD PTR \[rdi\+zmm2\*8\]
[ 	]*39[ 	]+vgatherqpd zmm6\{k1\}, ZMMWORD PTR \[edi\]
[ 	]*40[ 	]+vgatherqpd zmm6\{k1\}, ZMMWORD PTR \[zmm2\+eiz\]
[ 	]*41[ 	]+vpscatterdd ZMMWORD PTR \[edi\]\{k1\}, zmm6
[ 	]*42[ 	]+vpscatterdd ZMMWORD PTR \[zmm2\+eiz\]\{k1\}, zmm6
[ 	]*43[ 	]+
[ 	]*44[ 	]+vaddps zmm2, zmm1, QWORD PTR \[eax\]\{1to8\}
[ 	]*45[ 	]+vaddps zmm2, zmm1, QWORD PTR \[eax\]\{1to16\}
[ 	]*46[ 	]+vaddpd zmm2, zmm1, DWORD PTR \[eax\]\{1to8\}
[ 	]*47[ 	]+vaddpd zmm2, zmm1, DWORD PTR \[eax\]\{1to16\}
[ 	]*48[ 	]+vaddps zmm2, zmm1, ZMMWORD PTR \[eax\]\{1to16\}
[ 	]*49[ 	]+vaddps zmm2, zmm1, DWORD PTR \[eax\]
[ 	]*50[ 	]+vaddpd zmm2, zmm1, QWORD PTR \[eax\]
