#source: ../x86-64-drx.s
#objdump: -dw
#name: x86-64 (ILP32) debug register related opcodes

.*: +file format .*

Disassembly of section .text:

0+ <_start>:
[ 	]*[0-9a-f]+:	44 0f 21 c0[ 	]+movq?[ 	]+?%db8,%rax
[ 	]*[0-9a-f]+:	44 0f 21 c7[ 	]+movq?[ 	]+?%db8,%rdi
[ 	]*[0-9a-f]+:	44 0f 23 c0[ 	]+movq?[ 	]+?%rax,%db8
[ 	]*[0-9a-f]+:	44 0f 23 c7[ 	]+movq?[ 	]+?%rdi,%db8
[ 	]*[0-9a-f]+:	44 0f 21 c0[ 	]+movq?[ 	]+?%db8,%rax
[ 	]*[0-9a-f]+:	44 0f 21 c7[ 	]+movq?[ 	]+?%db8,%rdi
[ 	]*[0-9a-f]+:	44 0f 23 c0[ 	]+movq?[ 	]+?%rax,%db8
[ 	]*[0-9a-f]+:	44 0f 23 c7[ 	]+movq?[ 	]+?%rdi,%db8
[ 	]*[0-9a-f]+:	44 0f 21 c0[ 	]+movq?[ 	]+?%db8,%rax
[ 	]*[0-9a-f]+:	44 0f 21 c7[ 	]+movq?[ 	]+?%db8,%rdi
[ 	]*[0-9a-f]+:	44 0f 23 c0[ 	]+movq?[ 	]+?%rax,%db8
[ 	]*[0-9a-f]+:	44 0f 23 c7[ 	]+movq?[ 	]+?%rdi,%db8
