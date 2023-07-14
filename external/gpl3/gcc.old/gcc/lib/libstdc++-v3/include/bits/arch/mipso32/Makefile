#	$NetBSD: Makefile,v 1.1.1.1 2023/07/14 06:34:27 mrg Exp $

GCC_MACHINE_ARCH=mipso32
.include "../Makefile.arch_bits"

# mips o32 files are in mipse[lb] subdirs.
.PATH: ${.CURDIR}/../../../../arch/${MACHINE_ARCH:S/mipsn/mips/:S/64//}
