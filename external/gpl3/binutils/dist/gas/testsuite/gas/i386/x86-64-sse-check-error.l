.*: Assembler messages:
.*:7: Error: .*
.*:10: Error: .*
.*:13: Error: .*
.*:16: Error: .*
.*:19: Error: .*
.*:20: Error: .*
GAS LISTING .*


[ 	]*1[ 	]+\.include "sse-check\.s"
[ 	]*1[ 	]+\# Check SSE instructions
[ 	]*2[ 	]+
[ 	]*3[ 	]+\.text
[ 	]*4[ 	]+_start:
[ 	]*5[ 	]+
[ 	]*6[ 	]+\# SSE instruction
[ 	]*7[ 	]+\?\?\?\? 0F58CA   		addps %xmm2,%xmm1
.*  Error: SSE instruction `addps' is used
[ 	]*8[ 	]+
[ 	]*9[ 	]+\# SSE2 instruction
[ 	]*10[ 	]+\?\?\?\? 660F58CA 		addpd %xmm2,%xmm1
.*  Error: SSE instruction `addpd' is used
[ 	]*11[ 	]+
[ 	]*12[ 	]+\# SSE3 instruction
[ 	]*13[ 	]+\?\?\?\? 660FD0CA 		addsubpd %xmm2,%xmm1
.*  Error: SSE instruction `addsubpd' is used
[ 	]*14[ 	]+
[ 	]*15[ 	]+\# SSSE3 instruction
[ 	]*16[ 	]+\?\?\?\? 660F3801 		phaddw %xmm2,%xmm1
.*  Error: SSE instruction `phaddw' is used
[ 	]*16[ 	]+CA
[ 	]*17[ 	]+
[ 	]*18[ 	]+\# SSE4 instructions
[ 	]*19[ 	]+\?\?\?\? 660F3815 		blendvpd %xmm0,%xmm1,%xmm0
.*  Error: SSE instruction `blendvpd' is used
[ 	]*19[ 	]+C1
[ 	]*20[ 	]+\?\?\?\? 660F3837 		pcmpgtq %xmm1,%xmm0
.*  Error: SSE instruction `pcmpgtq' is used
[ 	]*20[ 	]+C1
