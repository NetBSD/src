 .globl my_func
 .type my_func,@function
 .section .opd,"aw",@progbits
my_func:
 .quad .Lmy_func, .TOC.@tocbase

 .text
.Lmy_func:
 blr
 .size my_func,.-.Lmy_func
