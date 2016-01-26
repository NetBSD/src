#NO_APP
 .globl d
d:
 .long 21
#APP
 .if 0
#NO_APP
 .err
 .globl x
x:
#APP
 .endif
