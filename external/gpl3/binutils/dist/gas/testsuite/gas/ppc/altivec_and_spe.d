#as: -maltivec -mspe -mppc64 
#objdump: -d -Mppc64
#name: Check that ISA extensions can be specified before CPU selection

.*: +file format elf.*-powerpc.*

Disassembly of section \.text:

0+00 <.*>:
   0:	7e 00 06 6c 	dssall
   4:	7d 00 83 a6 	mtspr   512,r8
   8:	4c 00 00 24 	rfid
