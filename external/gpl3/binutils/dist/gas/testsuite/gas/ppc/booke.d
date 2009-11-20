#as: -mbooke
#objdump: -dr -Mbooke
#name: BookE tests

.*: +file format elf(32)?(64)?-powerpc.*

Disassembly of section \.text:

0+0000000 <branch_target_1>:
   0:	7c a8 48 2c 	icbt    5,r8,r9
   4:	7c a6 02 26 	mfapidi r5,r6
   8:	7c 07 46 24 	tlbivax r7,r8
   c:	7c 0b 67 24 	tlbsx   r11,r12
  10:	7c 00 07 a4 	tlbwe   
  14:	7c 00 07 a4 	tlbwe   
  18:	7c 21 0f a4 	tlbwe   r1,r1,1

0+000001c <branch_target_2>:
  1c:	4c 00 00 66 	rfci
  20:	7c 60 01 06 	wrtee   r3
  24:	7c 00 81 46 	wrteei  1
  28:	7c 85 02 06 	mfdcrx  r4,r5
  2c:	7c aa 3a 86 	mfdcr   r5,234
  30:	7c e6 03 06 	mtdcrx  r6,r7
  34:	7d 10 6b 86 	mtdcr   432,r8
  38:	7c 00 04 ac 	msync
  3c:	7c 09 55 ec 	dcba    r9,r10
  40:	7c 00 06 ac 	mbar    
  44:	7c 00 06 ac 	mbar    
  48:	7c 20 06 ac 	mbar    1
  4c:	7d 8d 77 24 	tlbsx   r12,r13,r14
  50:	7d 8d 77 25 	tlbsx\.  r12,r13,r14
  54:	7c 12 42 a6 	mfsprg  r0,2
  58:	7c 12 42 a6 	mfsprg  r0,2
  5c:	7c 12 43 a6 	mtsprg  2,r0
  60:	7c 12 43 a6 	mtsprg  2,r0
  64:	7c 07 42 a6 	mfsprg  r0,7
  68:	7c 07 42 a6 	mfsprg  r0,7
  6c:	7c 17 43 a6 	mtsprg  7,r0
  70:	7c 17 43 a6 	mtsprg  7,r0
  74:	7c 05 32 2c 	dcbt    r5,r6
  78:	7c 05 32 2c 	dcbt    r5,r6
  7c:	7d 05 32 2c 	dcbt    8,r5,r6
