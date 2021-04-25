#	$NetBSD: Makefile.inc,v 1.8 2021/04/25 23:52:59 christos Exp $

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

.if !empty(MACHINE_ARCH:Mmips64*)
# For now make "elf32" look for native (n32)
MLIBDIR=	64
COMPAT_MLIBDIR=	o32
CPPFLAGS+= -DLDD_ELF64 -DMIPS_N32
.endif

.if !empty(MACHINE_ARCH:Mmipsn64*)
MLIBDIR=	n32
COMPAT_MLIBDIR=	o32
CPPFLAGS+= -DMIPS_N32
.endif

CPPFLAGS+=	-D_KERNTYPES

.endif	# MKCOMPAT

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
