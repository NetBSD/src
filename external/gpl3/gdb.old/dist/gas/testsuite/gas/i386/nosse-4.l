.*: Assembler messages:
.*:6: Error: .*generic.*
.*:9: Error: .*\.sse.*
.*:12: Error: .*\.sse2.*
.*:15: Error: .*\.sse3.*
.*:18: Error: .*\.ssse3.*
.*:21: Error: .*\.sse4\.1.*
.*:28: Error: .*\.nosse4.*
.*:32: Error: .*\.nosse4\.2.*
.*:35: Error: .*\.nosse4\.1.*
.*:38: Error: .*\.nossse3.*
.*:43: Error: .*\.nosse3.*
.*:45: Error: .*\.nommx.*
.*:47: Error: .*\.nosse2.*
.*:50: Error: .*\.nosse.*
GAS LISTING .*
#...
[ 	]*1[ 	]+\# Test \.arch \[\.sseX|\.nosseX\]
[ 	]*2[ 	]+\.text
[ 	]*3[ 	]+\.arch generic32
[ 	]*4[ 	]+\.arch \.mmx
[ 	]*5[ 	]+\?\?\?\? 0F77     		emms
[ 	]*6[ 	]+addps %xmm0, %xmm0
[ 	]*7[ 	]+\.arch \.sse
[ 	]*8[ 	]+\?\?\?\? 0F58C0   		addps %xmm0, %xmm0
[ 	]*9[ 	]+lfence
[ 	]*10[ 	]+\.arch \.sse2
[ 	]*11[ 	]+\?\?\?\? 0FAEE8   		lfence
[ 	]*12[ 	]+mwait
[ 	]*13[ 	]+\.arch \.sse3
[ 	]*14[ 	]+\?\?\?\? 0F01C9   		mwait
[ 	]*15[ 	]+pabsd %xmm0, %xmm0
[ 	]*16[ 	]+\.arch \.ssse3
[ 	]*17[ 	]+\?\?\?\? 660F381E 		pabsd %xmm0, %xmm0
[ 	]*17[ 	]+C0
[ 	]*18[ 	]+ptest %xmm0, %xmm0
[ 	]*19[ 	]+\.arch \.sse4\.1
[ 	]*20[ 	]+\?\?\?\? 660F3817 		ptest %xmm0, %xmm0
[ 	]*20[ 	]+C0
[ 	]*21[ 	]+crc32 %eax, %eax
[ 	]*22[ 	]+\.arch \.sse4\.2
[ 	]*23[ 	]+\?\?\?\? F20F38F1 		crc32 %eax, %eax
[ 	]*23[ 	]+C0
[ 	]*24[ 	]+\.arch \.nosse
[ 	]*25[ 	]+\.arch \.sse4
[ 	]*26[ 	]+\?\?\?\? F20F38F1 		crc32 %eax, %eax
[ 	]*26[ 	]+C0
[ 	]*27[ 	]+\.arch \.nosse4
[ 	]*28[ 	]+ptest %xmm0, %xmm0
[ 	]*29[ 	]+\?\?\?\? 660F381E 		pabsd %xmm0, %xmm0
[ 	]*29[ 	]+C0
[ 	]*30[ 	]+\.arch \.sse4
[ 	]*31[ 	]+\.arch \.nosse4\.2
[ 	]*32[ 	]+crc32 %eax, %eax
[ 	]*33[ 	]+\?\?\?\? 660F3817 		ptest %xmm0, %xmm0
[ 	]*33[ 	]+C0
[ 	]*34[ 	]+\.arch \.nosse4\.1
[ 	]*35[ 	]+ptest %xmm0, %xmm0
[ 	]*36[ 	]+\?\?\?\? 660F381E 		pabsd %xmm0, %xmm0
[ 	]*36[ 	]+C0
[ 	]*37[ 	]+\.arch \.nossse3
[ 	]*38[ 	]+pabsd %xmm0, %xmm0
[ 	]*39[ 	]+\?\?\?\? 0F01C9   		mwait
[ 	]*40[ 	]+\?\?\?\? 0F77     		emms
[ 	]*41[ 	]+\.arch \.nommx
[ 	]*42[ 	]+\.arch \.nosse3
[ 	]*43[ 	]+mwait
[ 	]*44[ 	]+\?\?\?\? 0FAEE8   		lfence
[ 	]*45[ 	]+emms
[ 	]*46[ 	]+\.arch \.nosse2
[ 	]*47[ 	]+lfence
[ 	]*48[ 	]+\?\?\?\? 0F58C0   		addps %xmm0, %xmm0
[ 	]*49[ 	]+\.arch \.nosse
[ 	]*50[ 	]+addps %xmm0, %xmm0
GAS LISTING .*


[ 	]*51[ 	]+\?\?\?\? 8DB42600 		\.p2align 4
[ 	]*51[ 	]+000000
#pass
