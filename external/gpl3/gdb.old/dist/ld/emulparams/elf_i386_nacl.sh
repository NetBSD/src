source_sh ${srcdir}/emulparams/elf_i386.sh
source_sh ${srcdir}/emulparams/elf_nacl.sh
OUTPUT_FORMAT="elf32-i386-nacl"
ARCH="i386:nacl"	# The :nacl just means one-byte nops for code fill.
