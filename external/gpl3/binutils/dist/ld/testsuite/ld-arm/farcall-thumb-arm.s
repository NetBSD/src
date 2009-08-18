@ Test to ensure that a Thumb to ARM call exceeding 4Mb generates a stub.

	.global _start
	.syntax unified

@ We will place the section .text at 0x1000.

	.text
	.thumb_func
_start:
	bl bar

@ We will place the section .foo at 0x2001014.

	.section .foo, "xa"

	.arm
	.type bar, %function
bar:
	bx lr

