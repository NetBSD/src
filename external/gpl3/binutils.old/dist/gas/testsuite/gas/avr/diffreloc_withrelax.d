#name: AVR DIFF relocs with link relax
#as: -mmcu=avrxmega2 -mlink-relax
#source: relax.s
#objdump: -r
#target: avr-*-*

.*:     file format elf32-avr

RELOCATION RECORDS FOR \[.text\]:
OFFSET   TYPE              VALUE 
00000000 R_AVR_CALL        .text


RELOCATION RECORDS FOR \[.data\]:
OFFSET   TYPE              VALUE 
00000000 R_AVR_DIFF16      .text\+0x00000004
