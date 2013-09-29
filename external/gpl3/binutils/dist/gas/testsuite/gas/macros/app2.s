#NO_APP
 .text
 .macro foo
 .globl a
a:
 .long 42
 .endm
#APP
 foo
 .globl b
b:
 .long 56
#NO_APP
