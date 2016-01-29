.text
    .globl     _start
    .type      _start, @function
_start:
    movl      $call1, %edi
    bnd call *%rdi
