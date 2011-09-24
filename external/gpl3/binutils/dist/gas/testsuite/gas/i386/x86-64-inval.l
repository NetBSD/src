.*: Assembler messages:
.*:3: Error: .*
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
.*:29: Error: .*
.*:30: Error: .*
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
.*:56: Error: .*
.*:57: Error: .*
.*:58: Error: .*
.*:59: Error: .*
.*:60: Error: .*
.*:61: Error: .*
.*:62: Error: .*
.*:63: Error: .*
.*:64: Error: .*
.*:66: Error: .*
.*:67: Error: .*
.*:70: Error: .*
.*:71: Error: .*
.*:72: Error: .*
.*:73: Error: .*
.*:74: Error: .*
.*:75: Error: .*
.*:76: Error: .*
.*:77: Error: .*
.*:78: Error: .*
.*:79: Error: .*
.*:80: Error: .*
.*:81: Error: .*
.*:82: Error: .*
.*:83: Error: .*
.*:84: Error: .*
.*:85: Error: .*
.*:86: Error: .*
.*:87: Error: .*
.*:88: Error: .*
.*:89: Error: .*
.*:90: Error: .*
.*:91: Error: .*
.*:92: Error: .*
.*:93: Error: .*
.*:94: Error: .*
.*:95: Error: .*
.*:96: Error: .*
.*:97: Error: .*
.*:98: Error: .*
.*:99: Error: .*
.*:100: Error: .*
.*:101: Error: .*
.*:102: Error: .*
.*:103: Error: .*
GAS LISTING .*


[ 	]*1[ 	]+\.text
[ 	]*2[ 	]+\# All the following should be illegal for x86-64
[ 	]*3[ 	]+aaa		\# illegal
[ 	]*4[ 	]+aad		\# illegal
[ 	]*5[ 	]+aam		\# illegal
[ 	]*6[ 	]+aas		\# illegal
[ 	]*7[ 	]+arpl %ax,%ax	\# illegal
[ 	]*8[ 	]+bound %eax,\(%rax\) \# illegal
[ 	]*9[ 	]+calll \*%eax	\# 32-bit data size not allowed
[ 	]*10[ 	]+calll \*\(%ax\)	\# 32-bit data size not allowed
[ 	]*11[ 	]+calll \*\(%eax\)	\# 32-bit data size not allowed
[ 	]*12[ 	]+calll \*\(%r8\)	\# 32-bit data size not allowed
[ 	]*13[ 	]+calll \*\(%rax\)	\# 32-bit data size not allowed
[ 	]*14[ 	]+callq \*\(%ax\)	\# no 16-bit addressing
[ 	]*15[ 	]+daa		\# illegal
[ 	]*16[ 	]+das		\# illegal
[ 	]*17[ 	]+enterl \$0,\$0	\# can't have 32-bit stack operands
[ 	]*18[ 	]+into		\# illegal
[ 	]*19[ 	]+foo:	jcxz foo	\# No prefix exists to select CX as a counter
[ 	]*20[ 	]+jmpl \*%eax	\# 32-bit data size not allowed
[ 	]*21[ 	]+jmpl \*\(%ax\)	\# 32-bit data size not allowed
[ 	]*22[ 	]+jmpl \*\(%eax\)	\# 32-bit data size not allowed
[ 	]*23[ 	]+jmpl \*\(%r8\)	\# 32-bit data size not allowed
[ 	]*24[ 	]+jmpl \*\(%rax\)	\# 32-bit data size not allowed
[ 	]*25[ 	]+jmpq \*\(%ax\)	\# no 16-bit addressing
[ 	]*26[ 	]+lcalll \$0,\$0	\# illegal
[ 	]*27[ 	]+lcallq \$0,\$0	\# illegal
[ 	]*28[ 	]+ldsl %eax,\(%rax\) \# illegal
[ 	]*29[ 	]+ldsq %rax,\(%rax\) \# illegal
[ 	]*30[ 	]+lesl %eax,\(%rax\) \# illegal
[ 	]*31[ 	]+lesq %rax,\(%rax\) \# illegal
[ 	]*32[ 	]+ljmpl \$0,\$0	\# illegal
[ 	]*33[ 	]+ljmpq \$0,\$0	\# illegal
[ 	]*34[ 	]+ljmpq \*\(%rax\)	\# 64-bit data size not allowed
[ 	]*35[ 	]+loopw foo	\# No prefix exists to select CX as a counter
[ 	]*36[ 	]+loopew foo	\# No prefix exists to select CX as a counter
[ 	]*37[ 	]+loopnew foo	\# No prefix exists to select CX as a counter
[ 	]*38[ 	]+loopnzw foo	\# No prefix exists to select CX as a counter
[ 	]*39[ 	]+loopzw foo	\# No prefix exists to select CX as a counter
[ 	]*40[ 	]+leavel		\# can't have 32-bit stack operands
[ 	]*41[ 	]+pop %ds		\# illegal
[ 	]*42[ 	]+pop %es		\# illegal
[ 	]*43[ 	]+pop %ss		\# illegal
[ 	]*44[ 	]+popa		\# illegal
[ 	]*45[ 	]+popl %eax	\# can't have 32-bit stack operands
[ 	]*46[ 	]+push %cs	\# illegal
[ 	]*47[ 	]+push %ds	\# illegal
[ 	]*48[ 	]+push %es	\# illegal
[ 	]*49[ 	]+push %ss	\# illegal
[ 	]*50[ 	]+pusha		\# illegal
[ 	]*51[ 	]+pushl %eax	\# can't have 32-bit stack operands
[ 	]*52[ 	]+pushfl		\# can't have 32-bit stack operands
[ 	]*53[ 	]+popfl		\# can't have 32-bit stack operands
[ 	]*54[ 	]+retl		\# can't have 32-bit stack operands
[ 	]*55[ 	]+insertq \$4,\$2,%xmm2,%ebx \# The last operand must be XMM register\.
[ 	]*56[ 	]+fnstsw %eax
[ 	]*57[ 	]+fnstsw %al
GAS LISTING .*


[ 	]*58[ 	]+fstsw %eax
[ 	]*59[ 	]+fstsw %al
[ 	]*60[ 	]+in \$8,%rax
[ 	]*61[ 	]+out %rax,\$8
[ 	]*62[ 	]+movzxl \(%rax\),%rax
[ 	]*63[ 	]+movnti %ax, \(%rax\)
[ 	]*64[ 	]+movntiw %ax, \(%rax\)
[ 	]*65[ 	]+
[ 	]*66[ 	]+mov 0x80000000\(%rax\),%ebx
[ 	]*67[ 	]+mov 0x80000000,%ebx
[ 	]*68[ 	]+
[ 	]*69[ 	]+\.intel_syntax noprefix
[ 	]*70[ 	]+cmpxchg16b dword ptr \[rax\] \# Must be oword
[ 	]*71[ 	]+movq xmm1, XMMWORD PTR \[rsp\]
[ 	]*72[ 	]+movq xmm1, DWORD PTR \[rsp\]
[ 	]*73[ 	]+movq xmm1, WORD PTR \[rsp\]
[ 	]*74[ 	]+movq xmm1, BYTE PTR \[rsp\]
[ 	]*75[ 	]+movq XMMWORD PTR \[rsp\],xmm1
[ 	]*76[ 	]+movq DWORD PTR \[rsp\],xmm1
[ 	]*77[ 	]+movq WORD PTR \[rsp\],xmm1
[ 	]*78[ 	]+movq BYTE PTR \[rsp\],xmm1
[ 	]*79[ 	]+fnstsw eax
[ 	]*80[ 	]+fnstsw al
[ 	]*81[ 	]+fstsw eax
[ 	]*82[ 	]+fstsw al
[ 	]*83[ 	]+in rax,8
[ 	]*84[ 	]+out 8,rax
[ 	]*85[ 	]+movsx ax, \[rax\]
[ 	]*86[ 	]+movsx eax, \[rax\]
[ 	]*87[ 	]+movsx rax, \[rax\]
[ 	]*88[ 	]+movzx ax, \[rax\]
[ 	]*89[ 	]+movzx eax, \[rax\]
[ 	]*90[ 	]+movzx rax, \[rax\]
[ 	]*91[ 	]+movnti word ptr \[rax\], ax
[ 	]*92[ 	]+calld eax	\# 32-bit data size not allowed
[ 	]*93[ 	]+calld \[ax\]	\# 32-bit data size not allowed
[ 	]*94[ 	]+calld \[eax\]	\# 32-bit data size not allowed
[ 	]*95[ 	]+calld \[r8\]	\# 32-bit data size not allowed
[ 	]*96[ 	]+calld \[rax\]	\# 32-bit data size not allowed
[ 	]*97[ 	]+callq \[ax\]	\# no 16-bit addressing
[ 	]*98[ 	]+jmpd eax	\# 32-bit data size not allowed
[ 	]*99[ 	]+jmpd \[ax\]	\# 32-bit data size not allowed
[ 	]*100[ 	]+jmpd \[eax\]	\# 32-bit data size not allowed
[ 	]*101[ 	]+jmpd \[r8\]	\# 32-bit data size not allowed
[ 	]*102[ 	]+jmpd \[rax\]	\# 32-bit data size not allowed
[ 	]*103[ 	]+jmpq \[ax\]	\# no 16-bit addressing
