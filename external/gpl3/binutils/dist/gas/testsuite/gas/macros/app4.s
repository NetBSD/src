 .text
 .macro foo
 .globl a
a:
 .long 42
 .endm
 .include "app4b.s"
 foo
 .globl b
b:
 .long 56
