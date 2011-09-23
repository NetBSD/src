#	$NetBSD: bsd.64.mk,v 1.9 2011/09/23 06:44:37 mrg Exp $

.if ${MACHINE_ARCH} == "mips64eb"
LD+=		-m elf64btsmip
.else
LD+=		-m elf64ltsmip
.endif
MLIBDIR=	64

# XXX
# GCC 4.5 libgomp wants a different omp.h installed for the 64 bit
# version of it, and we don't have a way of doing that yet.
NO_LIBGOMP=	1

COPTS+=		-mabi=64
CPUFLAGS+=	-mabi=64
LDADD+=		-mabi=64
LDFLAGS+=	-mabi=64
MKDEPFLAGS+=	-mabi=64

.include "${.PARSEDIR}/../../Makefile.compat"
