#as: --64
#ld: -pie -melf_x86_64 --hash-style=sysv -z max-page-size=0x200000 -z noseparate-code
#objdump: -dw

.*: +file format .*


Disassembly of section .text:

0+191 <_start>:
 +191:	48 8d 05 68 fe ff ff 	lea    -0x198\(%rip\),%rax        # 0 <_start-0x191>
#pass
