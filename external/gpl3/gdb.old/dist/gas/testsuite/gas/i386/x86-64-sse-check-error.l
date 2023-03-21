.*: Assembler messages:
.*:7: Error: .*
.*:10: Error: .*
.*:13: Error: .*
.*:16: Error: .*
.*:19: Error: .*
.*:20: Error: .*
.*:26: Error: .*
.*:29: Error: .*
.*:32: Error: .*
.*:35: Error: .*
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
[ 	]*21[ 	]+
[ 	]*22[ 	]+\# SSE4a instruction.*
[ 	]*23[ 	]+\?\?\?\? 660F78C0 		extrq \$0, \$0, %xmm0
[ 	]*23[ 	]+0000
[ 	]*24[ 	]+
[ 	]*25[ 	]+\# PCMUL instruction
[ 	]*26[ 	]+\?\?\?\? 660F3A44 		pclmulqdq \$-1,%xmm1,%xmm2
.*  Error: SSE instruction `pclmulqdq' is used
[ 	]*26[ 	]+D1FF
[ 	]*27[ 	]+
[ 	]*28[ 	]+\# AES instructions
[ 	]*29[ 	]+\?\?\?\? 660F38DE 		aesdec %xmm1,%xmm2
.*  Error: SSE instruction `aesdec' is used
[ 	]*29[ 	]+D1
[ 	]*30[ 	]+
[ 	]*31[ 	]+\# SHA instruction
[ 	]*32[ 	]+\?\?\?\? 0F38C8C0 		sha1nexte %xmm0, %xmm0
.*  Error: SSE instruction `sha1nexte' is used
[ 	]*33[ 	]+
[ 	]*34[ 	]+\# GFNI instructions
[ 	]*35[ 	]+\?\?\?\? 660F38CF 		gf2p8mulb %xmm1,%xmm2
.*  Error: SSE instruction `gf2p8mulb' is used
[ 	]*35[ 	]+D1
[ 	]*36[ 	]+\?\?\?\? 62F27D09 		vgf2p8mulb %xmm0, %xmm0, %xmm0\{%k1\}
[ 	]*36[ 	]+CFC0
[ 	]*37[ 	]+\?\?\?\? 62F27D48 		vgf2p8mulb %zmm0, %zmm0, %zmm0
[ 	]*37[ 	]+CFC0
GAS LISTING .*


