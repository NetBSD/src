#as: --64
#ld: -melf_x86_64 -shared -z max-page-size=0x200000
#objdump: -dw

.*: +file format .*


Disassembly of section .text:

[a-f0-9]+ <bar>:
[ 	]*[a-f0-9]+:	48 8b 05 ([0-9a-f]{2} ){4}	mov    0x[a-f0-9]+\(%rip\),%rax        # [a-f0-9]+ <_DYNAMIC\+0x[a-f0-9]+>
#pass
