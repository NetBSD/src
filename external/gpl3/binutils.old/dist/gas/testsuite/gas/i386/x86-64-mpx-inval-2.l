.*: Assembler messages:
.*:6: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:7: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:10: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:11: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:13: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:14: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:17: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:18: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:21: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:22: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:25: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:26: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:29: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:30: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:33: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:34: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:37: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:38: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:41: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:42: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:44: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:45: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:48: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:49: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:52: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:53: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:56: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:57: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:60: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:61: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:64: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:65: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
GAS LISTING .*


[ 	]*1[ 	]+\# MPX instructions
[ 	]*2[ 	]+\.allow_index_reg
[ 	]*3[ 	]+\.text
[ 	]*4[ 	]+
[ 	]*5[ 	]+\#\#\# bndmk
[ 	]*6[ 	]+\?\?\?\? 67F30F1B 		bndmk \(%eax\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*6[ 	]+08
[ 	]*7[ 	]+\?\?\?\? 67F30F1B 		bndmk 0x3\(%ecx,%ebx,1\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*7[ 	]+4C1903
[ 	]*8[ 	]+
[ 	]*9[ 	]+\#\#\# bndmov
[ 	]*10[ 	]+\?\?\?\? 6766410F 		bndmov \(%r8d\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*10[ 	]+1A08
[ 	]*11[ 	]+\?\?\?\? 6766410F 		bndmov 0x3\(%r9d,%edx,1\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*11[ 	]+1A4C1103 
[ 	]*12[ 	]+
[ 	]*13[ 	]+\?\?\?\? 67660F1B 		bndmov %bnd1, \(%eax\)
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*13[ 	]+08
[ 	]*14[ 	]+\?\?\?\? 67660F1B 		bndmov %bnd1, 0x3\(%ecx,%eax,1\)
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*14[ 	]+4C0103
[ 	]*15[ 	]+
[ 	]*16[ 	]+\#\#\# bndcl
[ 	]*17[ 	]+\?\?\?\? 67F30F1A 		bndcl \(%ecx\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*17[ 	]+09
[ 	]*18[ 	]+\?\?\?\? 67F30F1A 		bndcl 0x3\(%ecx,%eax,1\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*18[ 	]+4C0103
[ 	]*19[ 	]+
[ 	]*20[ 	]+\#\#\# bndcu
[ 	]*21[ 	]+\?\?\?\? 67F20F1A 		bndcu \(%ecx\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*21[ 	]+09
[ 	]*22[ 	]+\?\?\?\? 67F20F1A 		bndcu 0x3\(%ecx,%eax,1\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*22[ 	]+4C0103
[ 	]*23[ 	]+
[ 	]*24[ 	]+\#\#\# bndcn
[ 	]*25[ 	]+\?\?\?\? 67F20F1B 		bndcn \(%ecx\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*25[ 	]+09
[ 	]*26[ 	]+\?\?\?\? 67F20F1B 		bndcn 0x3\(%ecx,%eax,1\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*26[ 	]+4C0103
[ 	]*27[ 	]+
[ 	]*28[ 	]+\#\#\# bndstx
[ 	]*29[ 	]+\?\?\?\? 670F1B44 		bndstx %bnd0, 0x3\(%eax,%ebx,1\)
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*29[ 	]+1803
[ 	]*30[ 	]+\?\?\?\? 670F1B53 		bndstx %bnd2, 3\(%ebx,1\)
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
GAS LISTING .*


[ 	]*30[ 	]+03
[ 	]*31[ 	]+
[ 	]*32[ 	]+\#\#\# bndldx
[ 	]*33[ 	]+\?\?\?\? 670F1A44 		bndldx 0x3\(%eax,%ebx,1\), %bnd0
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*33[ 	]+1803
[ 	]*34[ 	]+\?\?\?\? 670F1A53 		bndldx 3\(%ebx,1\), %bnd2
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*34[ 	]+03
[ 	]*35[ 	]+
[ 	]*36[ 	]+\.intel_syntax noprefix
[ 	]*37[ 	]+\?\?\?\? 67F30F1B 		bndmk bnd1, \[eax\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*37[ 	]+08
[ 	]*38[ 	]+\?\?\?\? 67F30F1B 		bndmk bnd1, \[edx\+1\*eax\+0x3\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*38[ 	]+4C0203
[ 	]*39[ 	]+
[ 	]*40[ 	]+\#\#\# bndmov
[ 	]*41[ 	]+\?\?\?\? 67660F1A 		bndmov bnd1, \[eax\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*41[ 	]+08
[ 	]*42[ 	]+\?\?\?\? 67660F1A 		bndmov bnd1, \[edx\+1\*eax\+0x3\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*42[ 	]+4C0203
[ 	]*43[ 	]+
[ 	]*44[ 	]+\?\?\?\? 67660F1B 		bndmov \[eax\], bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*44[ 	]+08
[ 	]*45[ 	]+\?\?\?\? 67660F1B 		bndmov \[edx\+1\*eax\+0x3\], bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*45[ 	]+4C0203
[ 	]*46[ 	]+
[ 	]*47[ 	]+\#\#\# bndcl
[ 	]*48[ 	]+\?\?\?\? 67F30F1A 		bndcl bnd1, \[eax\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*48[ 	]+08
[ 	]*49[ 	]+\?\?\?\? 67F30F1A 		bndcl bnd1, \[edx\+1\*eax\+0x3\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*49[ 	]+4C0203
[ 	]*50[ 	]+
[ 	]*51[ 	]+\#\#\# bndcu
[ 	]*52[ 	]+\?\?\?\? 67F20F1A 		bndcu bnd1, \[eax\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*52[ 	]+08
[ 	]*53[ 	]+\?\?\?\? 67F20F1A 		bndcu bnd1, \[edx\+1\*eax\+0x3\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*53[ 	]+4C0203
[ 	]*54[ 	]+
[ 	]*55[ 	]+\#\#\# bndcn
[ 	]*56[ 	]+\?\?\?\? 67F20F1B 		bndcn bnd1, \[eax\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*56[ 	]+08
[ 	]*57[ 	]+\?\?\?\? 67F20F1B 		bndcn bnd1, \[edx\+1\*eax\+0x3\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*57[ 	]+4C0203
[ 	]*58[ 	]+
GAS LISTING .*


[ 	]*59[ 	]+\#\#\# bndstx
[ 	]*60[ 	]+\?\?\?\? 670F1B44 		bndstx \[eax\+ebx\*1\+0x3\], bnd0
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*60[ 	]+1803
[ 	]*61[ 	]+\?\?\?\? 670F1B14 		bndstx \[1\*ebx\+3\], bnd2
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*61[ 	]+1D030000 
[ 	]*61[ 	]+00
[ 	]*62[ 	]+
[ 	]*63[ 	]+\#\#\# bndldx
[ 	]*64[ 	]+\?\?\?\? 670F1A44 		bndldx bnd0, \[eax\+ebx\*1\+0x3\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*64[ 	]+1803
[ 	]*65[ 	]+\?\?\?\? 670F1A14 		bndldx bnd2, \[1\*ebx\+3\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*65[ 	]+1D030000 
[ 	]*65[ 	]+00(|909090 )
#pass
