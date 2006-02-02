# as: --march=v32 --em=criself
# ld: -m criself
# objdump: -d

# Check that 32-bit branches (PCREL:s) are relocated right.

.*:     file format elf32-us-cris

Disassembly of section \.text:

0+ <a>:
   0:	bf0e 0800 0000      	ba 8 <b>
   6:	5e82                	moveq 30,r8

0+8 <b>:
   8:	4312                	moveq 3,r1
   a:	bf0e f6ff ffff      	ba 0 <(a|___init__start)>
  10:	4db2                	moveq 13,r11
