#objdump: -dw
#name: x86_64 RDPID insns

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
[ 	]*[a-f0-9]+:[ 	]*f3 0f c7 f8[ 	]*rdpid  %rax
[ 	]*[a-f0-9]+:[ 	]*f3 41 0f c7 fa[ 	]*rdpid  %r10
#pass
