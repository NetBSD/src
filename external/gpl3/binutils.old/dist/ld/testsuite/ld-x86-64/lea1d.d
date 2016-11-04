#source: lea1.s
#as: --x32
#ld: -Bsymbolic -shared -melf32_x86_64
#objdump: -dw

.*: +file format .*


Disassembly of section .text:

#...
[ 	]*[a-f0-9]+:	48 8d 05 ([0-9a-f]{2} ){4} *	lea    -0x[a-f0-9]+\(%rip\),%rax        # [a-f0-9]+ <foo>
[ 	]*[a-f0-9]+:	48 8d 05 ([0-9a-f]{2} ){4} *	lea    0x[a-f0-9]+\(%rip\),%rax        # [a-f0-9]+ <bar>
[ 	]*[a-f0-9]+:	48 8d 05 ([0-9a-f]{2} ){4} *	lea    0x[a-f0-9]+\(%rip\),%rax        # [a-f0-9]+ <__start_my_section>
[ 	]*[a-f0-9]+:	48 8d 05 ([0-9a-f]{2} ){4} *	lea    0x[a-f0-9]+\(%rip\),%rax        # [a-f0-9]+ <__stop_my_section>
#pass
