#include <m68k/asm.h>

	.globl _C_LABEL(print_str),_C_LABEL(print_num)

Lprint_str:	jmp _C_LABEL(print_str)
Lprint_num:	jmp _C_LABEL(print_num)

_060FPSP_TEST:
        .long    Lprint_str - _060FPSP_TEST      
	.long    Lprint_num - _060FPSP_TEST
	.space   120
#include "ftest.S"

_060ISP_TEST:
        .long    Lprint_str - _060ISP_TEST      
	.long    Lprint_num - _060ISP_TEST
	.space   120
#include "itest.S"

ENTRY_NOPROFILE(itest)
	bra	_060ISP_TEST+128+0
ENTRY_NOPROFILE(ftest1)
	bra	_060FPSP_TEST+128+0
ENTRY_NOPROFILE(ftest2)
	bra	_060FPSP_TEST+128+8
ENTRY_NOPROFILE(ftest3)
	bra	_060FPSP_TEST+128+16

