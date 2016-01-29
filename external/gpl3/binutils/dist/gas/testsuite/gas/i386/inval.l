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
.*:59: Error: .*
.*:60: Error: .*
.*:62: Error: .*
.*:63: Error: .*
.*:64: Error: .*
.*:65: Error: .*
.*:68: Error: .*
.*:69: Error: .*
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
.*:85: Error: .*
.*:86: Error: .*
.*:87: Error: .*
.*:88: Error: .*
.*:90: Error: .*
GAS LISTING .*


[ 	]*1[ 	]+\.text
[ 	]*2[ 	]+\.allow_index_reg
[ 	]*3[ 	]+\# All the following should be illegal
[ 	]*4[ 	]+mov	\(%dx\),%al
[ 	]*5[ 	]+mov	\(%eax,%esp,2\),%al
[ 	]*6[ 	]+setae	%eax
[ 	]*7[ 	]+pushb	%ds
[ 	]*8[ 	]+popb	%ds
[ 	]*9[ 	]+pushb	%al
[ 	]*10[ 	]+popb	%al
[ 	]*11[ 	]+pushb	%ah
[ 	]*12[ 	]+popb	%ah
[ 	]*13[ 	]+pushb	%ax
[ 	]*14[ 	]+popb	%ax
[ 	]*15[ 	]+pushb	%eax
[ 	]*16[ 	]+popb	%eax
[ 	]*17[ 	]+movb	%ds,%ax
[ 	]*18[ 	]+movb	%ds,%eax
[ 	]*19[ 	]+movb	%ax,%ds
[ 	]*20[ 	]+movb	%eax,%ds
[ 	]*21[ 	]+movdb	%eax,%mm0
[ 	]*22[ 	]+movqb	0,%mm0
[ 	]*23[ 	]+ldsb	0,%eax
[ 	]*24[ 	]+setnew	0
[ 	]*25[ 	]+movdw	%eax,%mm0
[ 	]*26[ 	]+movqw	0,%mm0
[ 	]*27[ 	]+div	%cx,%al
[ 	]*28[ 	]+div	%cl,%ax
[ 	]*29[ 	]+div	%ecx,%al
[ 	]*30[ 	]+imul	10,%bx,%ecx
[ 	]*31[ 	]+imul	10,%bx,%al
[ 	]*32[ 	]+popab
[ 	]*33[ 	]+stil
[ 	]*34[ 	]+aaab
[ 	]*35[ 	]+cwdel
[ 	]*36[ 	]+cwdw
[ 	]*37[ 	]+callww	0
[ 	]*38[ 	]+foo:	jaw	foo
[ 	]*39[ 	]+jcxzw	foo
[ 	]*40[ 	]+jecxzl	foo
[ 	]*41[ 	]+loopb	foo
[ 	]*42[ 	]+xlatw	%es:%bx
[ 	]*43[ 	]+xlatl	%es:%bx
[ 	]*44[ 	]+intl	2
[ 	]*45[ 	]+int3b
[ 	]*46[ 	]+hltb
[ 	]*47[ 	]+fstb	%st\(0\)
[ 	]*48[ 	]+fcompll	28\(%ebp\)
[ 	]*49[ 	]+fldlw	\(%eax\)
[ 	]*50[ 	]+movl	\$%ebx,%eax
[ 	]*51[ 	]+insertq	\$4,\$2,%xmm2,%ebx
[ 	]*52[ 	]+cvtsi2ssq \(%eax\),%xmm1
[ 	]*53[ 	]+cvtsi2sdq \(%eax\),%xmm1
[ 	]*54[ 	]+fnstsw %eax
[ 	]*55[ 	]+fnstsw %al
[ 	]*56[ 	]+fstsw %eax
[ 	]*57[ 	]+fstsw %al
GAS LISTING .*


[ 	]*58[ 	]+
[ 	]*59[ 	]+movnti %ax, \(%eax\)
[ 	]*60[ 	]+movntiw %ax, \(%eax\)
[ 	]*61[ 	]+
[ 	]*62[ 	]+add \(%si,%esi\), %eax
[ 	]*63[ 	]+add \(%esi,%si\), %eax
[ 	]*64[ 	]+add \(%eiz\), %eax
[ 	]*65[ 	]+add \(%eax\), %eiz
[ 	]*66[ 	]+
[ 	]*67[ 	]+\.intel_syntax noprefix
[ 	]*68[ 	]+cvtsi2ss xmm1,QWORD PTR \[eax\]
[ 	]*69[ 	]+cvtsi2sd xmm1,QWORD PTR \[eax\]
[ 	]*70[ 	]+cvtsi2ssq xmm1,QWORD PTR \[eax\]
[ 	]*71[ 	]+cvtsi2sdq xmm1,QWORD PTR \[eax\]
[ 	]*72[ 	]+movq xmm1, XMMWORD PTR \[esp\]
[ 	]*73[ 	]+movq xmm1, DWORD PTR \[esp\]
[ 	]*74[ 	]+movq xmm1, WORD PTR \[esp\]
[ 	]*75[ 	]+movq xmm1, BYTE PTR \[esp\]
[ 	]*76[ 	]+movq XMMWORD PTR \[esp\],xmm1
[ 	]*77[ 	]+movq DWORD PTR \[esp\],xmm1
[ 	]*78[ 	]+movq WORD PTR \[esp\],xmm1
[ 	]*79[ 	]+movq BYTE PTR \[esp\],xmm1
[ 	]*80[ 	]+fnstsw eax
[ 	]*81[ 	]+fnstsw al
[ 	]*82[ 	]+fstsw eax
[ 	]*83[ 	]+fstsw al
[ 	]*84[ 	]+
[ 	]*85[ 	]+movsx ax, \[eax\]
[ 	]*86[ 	]+movsx eax, \[eax\]
[ 	]*87[ 	]+movzx ax, \[eax\]
[ 	]*88[ 	]+movzx eax, \[eax\]
[ 	]*89[ 	]+
[ 	]*90[ 	]+movnti word ptr \[eax\], ax
