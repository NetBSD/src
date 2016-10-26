. ${srcdir}/emulparams/elf64alpha.sh
ENTRY=__start

NOP=0x47ff041f
# XXX binutils 2.13
# Note that the number is always big-endian, thus we have to 
# reverse the digit string.
#NOP=0x0000fe2f1f04ff47		# unop; nop
