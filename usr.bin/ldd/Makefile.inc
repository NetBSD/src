#	$NetBSD: Makefile.inc,v 1.5.8.1 2015/06/29 17:20:04 snj Exp $

WARNS?=	3	# XXX: -Wsign-compare issues ld.elf_so source

.if ${MKCOMPAT} != "no"

.if ${MACHINE_ARCH} == "sparc64"
MLIBDIR=	sparc
.endif

.if ${MACHINE_ARCH} == "x86_64"
MLIBDIR=	i386
.endif

.if ${MACHINE_ARCH} == "powerpc64"
MLIBDIR=	powerpc
.endif

# For now make "elf32" look for native (n32)
.if (${MACHINE_ARCH} == "mips64eb") || (${MACHINE_ARCH} == "mips64el")
MLIBDIR=	64
COMPAT_MLIBDIR=	o32
CPPFLAGS+= -DLDD_ELF64 -DMIPS_N32
.endif

.endif	# MKCOMPAT

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
