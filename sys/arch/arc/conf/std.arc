#	$NetBSD: std.arc,v 1.16 2003/08/30 22:44:38 chs Exp $
# standard arc info

machine arc mips
makeoptions	MACHINE_ARCH="mipsel"

mainbus0 at root
cpu* at mainbus0

# set CPU architecture level for kernel target
#options 	MIPS1			# R2000/R3000 support
options 	MIPS3			# R4000/R4400 support

# Standard (non-optional) system "options"

# Standard exec-package options
options 	EXEC_ELF32		# native exec format
options 	EXEC_SCRIPT		# may be unsafe

options 	MIPS3_L2CACHE_ABSENT	# may not have L2 cache

makeoptions	DEFTEXTADDR="0x80200000"
