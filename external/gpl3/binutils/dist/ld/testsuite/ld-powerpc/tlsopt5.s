 .globl _start
_start:
 addi 3,2,gd@got@tlsgd
 bl __tls_get_addr(gd@tlsgd)
 nop
