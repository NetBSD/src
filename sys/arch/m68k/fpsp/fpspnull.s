| dummy FPSP entries

	.text

	.globl	fpsp_unimp
	.globl	fpsp_bsun,fpsp_unfl,fpsp_operr
	.globl	fpsp_ovfl,fpsp_snan,fpsp_unsupp

fpsp_unimp:
fpsp_bsun:
fpsp_unfl:
fpsp_operr:
fpsp_ovfl:
fpsp_snan:
fpsp_unsupp:
	pea	LnoFPSP
	jbsr	_panic
LnoFPSP:
	.asciz	"68040 FPSP not installed"
