	.section	.data.exit,"aw"
data:
	.section	.text.exit,"aw"
text:
	.text
	.globl _start
_start:
	.long	data
	.section	.debug_info
	.long	text
