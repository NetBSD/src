#	$NetBSD: Makefile,v 1.2 2021/04/25 23:12:54 christos Exp $

GCC_MACHINE_ARCH=mipso32
.include "../Makefile.arch_bits"

# mips o32 files are in mipse[lb] subdirs.
.PATH: ${.CURDIR}/../../../../arch/${MACHINE_ARCH:S/mipsn/mips/:S/64//}
