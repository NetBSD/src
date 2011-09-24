.*: Assembler messages:
.*:4: Error: .*
GAS LISTING .*


[ 	]*1[ 	]+\.include "arch-avx-1\.s"
[ 	]*1[ 	]+\# Test -march=
[ 	]*2[ 	]+\.text
[ 	]*3[ 	]+\# AES \+ AVX
[ 	]*4[ 	]+vaesenc  \(%ecx\),%xmm0,%xmm2
[ 	]*5[ 	]+\# PCLMUL \+ AVX
[ 	]*6[ 	]+\?\?\?\? C4E34944 	vpclmulqdq \$8,%xmm4,%xmm6,%xmm2
[ 	]*6[ 	]+D408
