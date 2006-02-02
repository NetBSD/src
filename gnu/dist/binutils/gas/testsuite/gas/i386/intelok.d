#as: -J
#objdump: -drwMintel
#name: i386 intel-ok

.*: +file format .*

Disassembly of section .text:

0+000 <start>:
[ 	]*[0-9a-f]+:[ 	]+02 00[ 	]+add[ 	]+al,(BYTE PTR )?\[eax\]
[ 	]*[0-9a-f]+:	02 00[ 	]+add[ 	]+al,(BYTE PTR )?\[eax\]
[ 	]*[0-9a-f]+:	66 03 00[ 	]+add[ 	]+ax,(WORD PTR )?\[eax\]
[ 	]*[0-9a-f]+:	66 03 00[ 	]+add[ 	]+ax,(WORD PTR )?\[eax\]
[ 	]*[0-9a-f]+:	03 00[ 	]+add[ 	]+eax,(DWORD PTR )?\[eax\]
[ 	]*[0-9a-f]+:	03 00[ 	]+add[ 	]+eax,(DWORD PTR )?\[eax\]
[ 	]*[0-9a-f]+:	80 00 01[ 	]+add[ 	]+BYTE PTR \[eax\],0x1
[ 	]*[0-9a-f]+:	83 00 01[ 	]+add[ 	]+DWORD PTR \[eax\],0x1
[ 	]*[0-9a-f]+:	66 83 00 01[ 	]+add[ 	]+WORD PTR \[eax\],0x1
[ 	]*[0-9a-f]+:	66 0f 58 00[ 	]+addpd[ 	]+xmm0,XMMWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	66 0f 58 00[ 	]+addpd[ 	]+xmm0,XMMWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	0f 58 00[ 	]+addps[ 	]+xmm0,XMMWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	0f 58 00[ 	]+addps[ 	]+xmm0,XMMWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	f2 0f 58 00[ 	]+addsd[ 	]+xmm0,QWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	f2 0f 58 00[ 	]+addsd[ 	]+xmm0,QWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	f3 0f 58 00[ 	]+addss[ 	]+xmm0,DWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	f3 0f 58 00[ 	]+addss[ 	]+xmm0,DWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	66 ff 10[ 	]+call[ 	]+WORD PTR \[eax\]
[ 	]*[0-9a-f]+:	ff 10[ 	]+call[ 	]+DWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	ff 18[ 	]+call[ 	]+FWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	a6[ 	]+cmps[ 	]+(BYTE PTR )(ds:)?\[esi\],(\1)?es:\[edi\]
[ 	]*[0-9a-f]+:	a7[ 	]+cmps[ 	]+(DWORD PTR )(ds:)?\[esi\],(\1)?es:\[edi\]
[ 	]*[0-9a-f]+:	66 a7[ 	]+cmps[ 	]+(WORD PTR )(ds:)?\[esi\],(\1)?es:\[edi\]
[ 	]*[0-9a-f]+:	0f c7 08[ 	]+cmpxchg8b[ 	]+(QWORD PTR )?\[eax\]
[ 	]*[0-9a-f]+:	d8 00[ 	]+fadd[ 	]+DWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	dc 00[ 	]+fadd[ 	]+QWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	df 20[ 	]+fbld[ 	]+(TBYTE PTR )?\[eax\]
[ 	]*[0-9a-f]+:	df 20[ 	]+fbld[ 	]+(TBYTE PTR )?\[eax\]
[ 	]*[0-9a-f]+:	df 30[ 	]+fbstp[ 	]+(TBYTE PTR )?\[eax\]
[ 	]*[0-9a-f]+:	df 30[ 	]+fbstp[ 	]+(TBYTE PTR )?\[eax\]
[ 	]*[0-9a-f]+:	da 00[ 	]+fiadd[ 	]+DWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	de 00[ 	]+fiadd[ 	]+WORD PTR \[eax\]
[ 	]*[0-9a-f]+:	db 00[ 	]+fild[ 	]+DWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	df 28[ 	]+fild[ 	]+QWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	df 00[ 	]+fild[ 	]+WORD PTR \[eax\]
[ 	]*[0-9a-f]+:	db 10[ 	]+fist[ 	]+DWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	df 10[ 	]+fist[ 	]+WORD PTR \[eax\]
[ 	]*[0-9a-f]+:	db 18[ 	]+fistp[ 	]+DWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	df 38[ 	]+fistp[ 	]+QWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	df 18[ 	]+fistp[ 	]+WORD PTR \[eax\]
[ 	]*[0-9a-f]+:	db 08[ 	]+fisttp[ 	]+DWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	dd 08[ 	]+fisttp[ 	]+QWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	df 08[ 	]+fisttp[ 	]+WORD PTR \[eax\]
[ 	]*[0-9a-f]+:	d9 00[ 	]+fld[ 	]+DWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	dd 00[ 	]+fld[ 	]+QWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	db 28[ 	]+fld[ 	]+TBYTE PTR \[eax\]
[ 	]*[0-9a-f]+:	d9 28[ 	]+fldcw[ 	]+(WORD PTR )?\[eax\]
[ 	]*[0-9a-f]+:	d9 28[ 	]+fldcw[ 	]+(WORD PTR )?\[eax\]
[ 	]*[0-9a-f]+:	d9 20[ 	]+fldenvd?[ 	]+\[eax\]
[ 	]*[0-9a-f]+:	d9 20[ 	]+fldenvd?[ 	]+\[eax\]
[ 	]*[0-9a-f]+:	66 d9 20[ 	]+fldenvw[ 	]+\[eax\]
[ 	]*[0-9a-f]+:	d9 10[ 	]+fst[ 	]+DWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	dd 10[ 	]+fst[ 	]+QWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	d9 18[ 	]+fstp[ 	]+DWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	dd 18[ 	]+fstp[ 	]+QWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	db 38[ 	]+fstp[ 	]+TBYTE PTR \[eax\]
[ 	]*[0-9a-f]+:	66 c5 00[ 	]+lds[ 	]+ax,(DWORD PTR )?\[eax\]
[ 	]*[0-9a-f]+:	c5 00[ 	]+lds[ 	]+eax,(FWORD PTR )?\[eax\]
[ 	]*[0-9a-f]+:	66 c5 00[ 	]+lds[ 	]+ax,(DWORD PTR )?\[eax\]
[ 	]*[0-9a-f]+:	c5 00[ 	]+lds[ 	]+eax,(FWORD PTR )?\[eax\]
[ 	]*[0-9a-f]+:	8d 00[ 	]+lea[ 	]+eax,\[eax\]
[ 	]*[0-9a-f]+:	8d 00[ 	]+lea[ 	]+eax,\[eax\]
[ 	]*[0-9a-f]+:	8d 00[ 	]+lea[ 	]+eax,\[eax\]
[ 	]*[0-9a-f]+:	8d 00[ 	]+lea[ 	]+eax,\[eax\]
[ 	]*[0-9a-f]+:	8d 00[ 	]+lea[ 	]+eax,\[eax\]
[ 	]*[0-9a-f]+:	8d 00[ 	]+lea[ 	]+eax,\[eax\]
[ 	]*[0-9a-f]+:	8d 00[ 	]+lea[ 	]+eax,\[eax\]
[ 	]*[0-9a-f]+:	8d 00[ 	]+lea[ 	]+eax,\[eax\]
[ 	]*[0-9a-f]+:	0f 01 10[ 	]+lgdtd?[ 	]+(PWORD PTR)?\[eax\]
[ 	]*[0-9a-f]+:	0f 01 10[ 	]+lgdtd?[ 	]+(PWORD PTR)?\[eax\]
[ 	]*[0-9a-f]+:	66 0f 01 10[ 	]+lgdtw[ 	]+(PWORD PTR)?\[eax\]
[ 	]*[0-9a-f]+:	a4[ 	]+movs[ 	]+(BYTE PTR )es:\[edi\],(\1)?(ds:)?\[esi\]
[ 	]*[0-9a-f]+:	a5[ 	]+movs[ 	]+(DWORD PTR )es:\[edi\],(\1)?(ds:)?\[esi\]
[ 	]*[0-9a-f]+:	66 a5[ 	]+movs[ 	]+(WORD PTR )es:\[edi\],(\1)?(ds:)?\[esi\]
[ 	]*[0-9a-f]+:	0f be 00[ 	]+movsx[ 	]+eax,BYTE PTR \[eax\]
[ 	]*[0-9a-f]+:	0f bf 00[ 	]+movsx[ 	]+eax,WORD PTR \[eax\]
[ 	]*[0-9a-f]+:	0f fc 00[ 	]+paddb[ 	]+mm0,(QWORD PTR )?\[eax\]
[ 	]*[0-9a-f]+:	0f fc 00[ 	]+paddb[ 	]+mm0,(QWORD PTR )?\[eax\]
[ 	]*[0-9a-f]+:	66 0f fc 00[ 	]+paddb[ 	]+xmm0,(XMMWORD PTR )?\[eax\]
[ 	]*[0-9a-f]+:	66 0f fc 00[ 	]+paddb[ 	]+xmm0,(XMMWORD PTR )?\[eax\]
[ 	]*[0-9a-f]+:	0f c4 00 03[ 	]+pinsrw[ 	]+mm0,(WORD PTR )?\[eax\],0x3
[ 	]*[0-9a-f]+:	66 0f c4 00 07[ 	]+pinsrw[ 	]+xmm0,(WORD PTR )?\[eax\],0x7
[ 	]*[0-9a-f]+:	ff 30[ 	]+push[ 	]+DWORD PTR \[eax\]
[ 	]*[0-9a-f]+:	d7[ 	]+xlat(b|[ 	]+(BYTE PTR )?(ds:)?\[ebx\])
[ 	]*[0-9a-f]+:	d7[ 	]+xlat(b|[ 	]+(BYTE PTR )?(ds:)?\[ebx\])
[ 	]*[0-9a-f]+:	d7[ 	]+xlat(b|[ 	]+(BYTE PTR )?(ds:)?\[ebx\])
[ 	]*[0-9a-f]+:	8b 80 00 00 00 00[ 	]+mov[ 	]+eax,(DWORD PTR )?\[eax\][ 	]+[0-9a-f]+:[ 	]+(R_386_|dir)?32[ 	]+byte
[ 	]*[0-9a-f]+:	8b 80 00 00 00 00[ 	]+mov[ 	]+eax,(DWORD PTR )?\[eax\][ 	]+[0-9a-f]+:[ 	]+(R_386_|dir)?32[ 	]+byte
[ 	]*[0-9a-f]+:	8b 40 04[ 	]+mov[ 	]+eax,(DWORD PTR )?\[eax\+4\]
[ 	]*[0-9a-f]+:	8b 40 04[ 	]+mov[ 	]+eax,(DWORD PTR )?\[eax\+4\]
[ 	]*[0-9a-f]+:	8b 80 00 00 00 00[ 	]+mov[ 	]+eax,(DWORD PTR )?\[eax\][ 	]+[0-9a-f]+:[ 	]+(R_386_|dir)?32[ 	]+fword
[ 	]*[0-9a-f]+:	8b 80 00 00 00 00[ 	]+mov[ 	]+eax,(DWORD PTR )?\[eax\][ 	]+[0-9a-f]+:[ 	]+(R_386_|dir)?32[ 	]+fword
[ 	]*[0-9a-f]+:	8b 80 04 00 00 00[ 	]+mov[ 	]+eax,(DWORD PTR )?\[eax\+4\][ 	]+[0-9a-f]+:[ 	]+(R_386_|dir)?32[ 	]+qword
[ 	]*[0-9a-f]+:	8b 80 04 00 00 00[ 	]+mov[ 	]+eax,(DWORD PTR )?\[eax\+4\][ 	]+[0-9a-f]+:[ 	]+(R_386_|dir)?32[ 	]+qword
[ 	]*[0-9a-f]+:	8b 80 08 00 00 00[ 	]+mov[ 	]+eax,(DWORD PTR )?\[eax\+8\][ 	]+[0-9a-f]+:[ 	]+(R_386_|dir)?32[ 	]+tbyte
[ 	]*[0-9a-f]+:	8b 80 08 00 00 00[ 	]+mov[ 	]+eax,(DWORD PTR )?\[eax\+8\][ 	]+[0-9a-f]+:[ 	]+(R_386_|dir)?32[ 	]+tbyte
#[ 	]*[0-9a-f]+:	8b 04 85 00 00 00 00[ 	]+mov[ 	]+eax,(DWORD PTR )?\[eax\*4\][ 	]+[0-9a-f]+:[ 	]+(R_386_|dir)?32[ 	]+word
#[ 	]*[0-9a-f]+:	8b 04 85 00 00 00 00[ 	]+mov[ 	]+eax,(DWORD PTR )?\[eax\*4\][ 	]+[0-9a-f]+:[ 	]+(R_386_|dir)?32[ 	]+word
#[ 	]*[0-9a-f]+:	8b 04 85 04 00 00 00[ 	]+mov[ 	]+eax,(DWORD PTR )?\[eax\*4\+4\][ 	]+[0-9a-f]+:[ 	]+(R_386_|dir)?32[ 	]+xmmword
#[ 	]*[0-9a-f]+:	8b 04 85 04 00 00 00[ 	]+mov[ 	]+eax,(DWORD PTR )?\[eax\*4\+4\][ 	]+[0-9a-f]+:[ 	]+(R_386_|dir)?32[ 	]+xmmword
[ 	]*[0-9a-f]+:	6a 01[ 	]+push[ 	]+0x1
[ 	]*[0-9a-f]+:	6a ff[ 	]+push[ 	]+0xffffffff
[ 	]*[0-9a-f]+:	6a fe[ 	]+push[ 	]+0xfffffffe
[ 	]*[0-9a-f]+:	6a 02[ 	]+push[ 	]+0x2
[ 	]*[0-9a-f]+:	6a 01[ 	]+push[ 	]+0x1
[ 	]*[0-9a-f]+:	6a 04[ 	]+push[ 	]+0x4
[ 	]*[0-9a-f]+:	6a 01[ 	]+push[ 	]+0x1
[ 	]*[0-9a-f]+:	6a 01[ 	]+push[ 	]+0x1
[ 	]*[0-9a-f]+:	6a 08[ 	]+push[ 	]+0x8
[ 	]*[0-9a-f]+:	6a 01[ 	]+push[ 	]+0x1
[ 	]*[0-9a-f]+:	6a 02[ 	]+push[ 	]+0x2
[ 	]*[0-9a-f]+:	6a 03[ 	]+push[ 	]+0x3
[ 	]*[0-9a-f]+:	6a 0d[ 	]+push[ 	]+0xd
#pass
