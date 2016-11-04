. ${srcdir}/emulparams/elf32_x86_64.sh
. ${srcdir}/emulparams/elf_nacl.sh
OUTPUT_FORMAT="elf32-x86-64-nacl"
ARCH="i386:x64-32:nacl"	# The :nacl just means one-byte nops for code fill.
