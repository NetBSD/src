#objdump: -dw
#name: i386 prefix

.*: +file format .*

Disassembly of section .text:

0+ <foo>:
[ 	]*[a-f0-9]+:	9b 26 67 d9 3c       	fstcw  %es:\(%si\)
[ 	]*[a-f0-9]+:	9b df e0             	fstsw  %ax
[ 	]*[a-f0-9]+:	9b df e0             	fstsw  %ax
[ 	]*[a-f0-9]+:	9b 67 df e0          	addr16 fstsw %ax
[ 	]*[a-f0-9]+:	36 67 66 f3 a7       	repz cmpsw %es:\(%di\),%ss:\(%si\)
[ 	]*[a-f0-9]+:	26 9b                	es fwait
[ 	]*[a-f0-9]+:	9b                   	fwait
[ 	]*[a-f0-9]+:	65 c7 05 00 00 00 00 00 00 00 00 	movl   \$0x0,%gs:0x0
[ 	]*[a-f0-9]+:	66 f2 0f 38 17       	data16 \(bad\) 
[ 	]*[a-f0-9]+:	f2 66 0f 54          	repnz \(bad\)
[ 	]*[a-f0-9]+:	f2 0f 54             	repnz \(bad\)
[ 	]*[a-f0-9]+:	f2 66 0f 11 22       	data16 movsd %xmm4,\(%edx\)
[ 	]*[a-f0-9]+:	f2 67 66 0f 11 22    	data16 movsd %xmm4,\(%bp,%si\)
[ 	]*[a-f0-9]+:	f2 67 f0 66 0f 11 22 	lock data16 movsd %xmm4,\(%bp,%si\)
[ 	]*[a-f0-9]+:	f3 66 0f 11 22       	data16 movss %xmm4,\(%edx\)
[ 	]*[a-f0-9]+:	f3 67 f0 66 0f 11 22 	lock data16 movss %xmm4,\(%bp,%si\)
[ 	]*[a-f0-9]+:	f3 67 f2 66 0f 11 22 	repz data16 movsd %xmm4,\(%bp,%si\)
[ 	]*[a-f0-9]+:	f3 66 3e 0f 11 22    	data16 movss %xmm4,%ds:\(%edx\)
[ 	]*[a-f0-9]+:	f2 66 36 0f 11 22    	data16 movsd %xmm4,%ss:\(%edx\)
[ 	]*[a-f0-9]+:	f3 f0 f2 66 36 0f 11 22 	repz lock data16 movsd %xmm4,%ss:\(%edx\)
[ 	]*[a-f0-9]+:	f2 66 3e 36 0f 11 22 	data16 ds movsd %xmm4,%ss:\(%edx\)
[ 	]*[a-f0-9]+:	f2 67 66 3e 36 0f 11 22 	data16 ds movsd %xmm4,%ss:\(%bp,%si\)
[ 	]*[a-f0-9]+:	f2 67 f0 66 3e 36 0f 11 22 	lock data16 ds movsd %xmm4,%ss:\(%bp,%si\)
[ 	]*[a-f0-9]+:	f3 66 3e 36 0f 11 22 	data16 ds movss %xmm4,%ss:\(%edx\)
[ 	]*[a-f0-9]+:	f3 f0 66 3e 36 0f 11 22 	lock data16 ds movss %xmm4,%ss:\(%edx\)
[ 	]*[a-f0-9]+:	f3 67 f2 66 3e 36 0f 11 22 	repz data16 ds movsd %xmm4,%ss:\(%bp,%si\)
[ 	]*[a-f0-9]+:	f2 66 90             	repnz xchg %ax,%ax
[ 	]*[a-f0-9]+:	f2 67 66 90          	repnz addr16 xchg %ax,%ax
[ 	]*[a-f0-9]+:	f2 67 f0 66 90       	repnz addr16 lock xchg %ax,%ax
[ 	]*[a-f0-9]+:	f3 66 90             	data16 pause 
[ 	]*[a-f0-9]+:	f3 67 f0 66 90       	addr16 lock data16 pause 
[ 	]*[a-f0-9]+:	f3 67 f2 66 90       	repz addr16 repnz xchg %ax,%ax
[ 	]*[a-f0-9]+:	f2 3e 90             	repnz ds nop
[ 	]*[a-f0-9]+:	f2 f0 67 3e 90       	repnz lock addr16 ds nop
[ 	]*[a-f0-9]+:	f3 3e 90             	ds pause 
[ 	]*[a-f0-9]+:	f3 66 3e 90          	data16 ds pause 
[ 	]*[a-f0-9]+:	f3 f0 3e 90          	lock ds pause 
[ 	]*[a-f0-9]+:	f3 f0 67 3e 90       	lock addr16 ds pause 
[ 	]*[a-f0-9]+:	f3 f2 67 3e 90       	repz repnz addr16 ds nop
[ 	]*[a-f0-9]+:	66 f0 36 90          	lock ss xchg %ax,%ax
[ 	]*[a-f0-9]+:	f2 36 90             	repnz ss nop
[ 	]*[a-f0-9]+:	f2 66 36 90          	repnz ss xchg %ax,%ax
[ 	]*[a-f0-9]+:	f2 f0 36 90          	repnz lock ss nop
[ 	]*[a-f0-9]+:	f2 f0 67 36 90       	repnz lock addr16 ss nop
[ 	]*[a-f0-9]+:	f3 36 90             	ss pause 
[ 	]*[a-f0-9]+:	f3 67 36 90          	addr16 ss pause 
[ 	]*[a-f0-9]+:	f3 f0 67 36 90       	lock addr16 ss pause 
[ 	]*[a-f0-9]+:	f3 f2 36 90          	repz repnz ss nop
[ 	]*[a-f0-9]+:	f3 f2 67 36 90       	repz repnz addr16 ss nop
[ 	]*[a-f0-9]+:	f3 f0 f2 66 36 90    	repz lock repnz ss xchg %ax,%ax
[ 	]*[a-f0-9]+:	66 3e 36 90          	ds ss xchg %ax,%ax
[ 	]*[a-f0-9]+:	67 66 3e 36 90       	addr16 ds ss xchg %ax,%ax
[ 	]*[a-f0-9]+:	67 f0 66 3e 36 90    	addr16 lock ds ss xchg %ax,%ax
[ 	]*[a-f0-9]+:	f3 66 3e 36 90       	data16 ds ss pause 
[ 	]*[a-f0-9]+:	f3 f0 66 3e 36 90    	lock data16 ds ss pause 
[ 	]*[a-f0-9]+:	f3 f2 67 3e 36 90    	repz repnz addr16 ds ss nop
[ 	]*[a-f0-9]+:	f3 67 f2 66 3e 36 90 	repz addr16 repnz ds ss xchg %ax,%ax
[ 	]*[a-f0-9]+:	f3 0f c7             	\(bad\)  
[ 	]*[a-f0-9]+:	f8                   	clc    
[ 	]*[a-f0-9]+:	90                   	nop
[ 	]*[a-f0-9]+:	f3 0f c7             	\(bad\)  
[ 	]*[a-f0-9]+:	f0 90                	lock nop
[ 	]*[a-f0-9]+:	f2 0f c7             	\(bad\)  
[ 	]*[a-f0-9]+:	f8                   	clc    
[ 	]*[a-f0-9]+:	90                   	nop
[ 	]*[a-f0-9]+:	f2 0f c7             	\(bad\)  
[ 	]*[a-f0-9]+:	f0 90                	lock nop
	...
#pass
