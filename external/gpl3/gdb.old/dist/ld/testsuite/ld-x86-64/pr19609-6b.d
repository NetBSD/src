#source: pr19609-6.s
#as: --64 -mrelax-relocations=yes
#ld: -melf_x86_64 --defsym foobar=0x80000000 --no-relax
#objdump: -dw

.*: +file format .*


Disassembly of section .text:

[a-f0-9]+ <_start>:
[ 	]*[a-f0-9]+:	48 8b 05 ([0-9a-f]{2} ){4} *	mov    0x[a-f0-9]+\(%rip\),%rax        # [a-f0-9]+ <.got>
#pass
