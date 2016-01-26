#source: start1.s
#source: tls-dso-tpoffgotcomm1.s
#as: --no-underscore --pic --em=criself -I$srcdir/$subdir
#ld: -m crislinux -shared
#objdump: -d -h -s -t -r -R -p

# Make sure we can link a file with TPOFFGOT relocs against common
# symbols.

.*:     file format elf32-cris

Program Header:
    LOAD off    0x0+ vaddr 0x0+ paddr 0x0+ align 2\*\*13
         filesz 0x0+1c8 memsz 0x0+1c8 flags r-x
    LOAD off .*
         filesz .*
 DYNAMIC off .*
         filesz .*
     TLS off    .* vaddr .* paddr .* align 2\*\*2
         filesz 0x0+ memsz 0x0+8 flags r--
#...
Sections:
#...
  7 .got          0+14  0+2240  0+2240  0+240  2\*\*2
                  CONTENTS, ALLOC, LOAD, DATA
SYMBOL TABLE:
#...
0+ g       \.tbss	0+4 foo
#...
0+4 g       \.tbss	0+4 bar

#...
Contents of section .got:
 2240 c8210000 00000000 00000000 00000000  .*
 2250 00000000                             .*

Disassembly of section \.text:

0+1b8 <_start>:
 1b8:	41b2                	moveq 1,\$r11
	\.\.\.

0+1bc <do_test>:
 1bc:	2f0e 0c00 0000      	add\.d c <bar\+0x8>,\$r0
 1c2:	1f1e 1000           	add\.w 0x10,\$r1
	\.\.\.
