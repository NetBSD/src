#	$NetBSD: Makefile.inc,v 1.3 2009/12/13 08:50:56 mrg Exp $

WARNS?=	3	# XXX: -Wsign-compare issues ld.elf_so source

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
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
