# Test to ensure that an Aarch64 call exceeding 128MB generates an error
# if the destination is of type STT_SECTION (eg non-global symbol)

	.global _start

# We will place the section .text at 0x1000.

	.text

_start:
	bl bar

# We will place the section .foo at 0x8001020.

	.section .foo, "xa"

bar:
	ret

