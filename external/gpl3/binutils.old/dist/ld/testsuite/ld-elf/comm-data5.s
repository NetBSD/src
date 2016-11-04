 .comm i,4,4

 .section .rodata,"a",%progbits
 .dc.a i

 .globl main
 .globl start
 .globl _start
 .globl __start
 .text
main:
start:
_start:
__start:
 .dc.a 0
