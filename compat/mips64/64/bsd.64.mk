#	$NetBSD: bsd.64.mk,v 1.7 2011/07/10 03:05:33 mrg Exp $

.if ${MACHINE_ARCH} == "mips64eb"
LD+=		-m elf64btsmip
.else
LD+=		-m elf64ltsmip
.endif
LIBGMP_MACHINE_ARCH=	${MACHINE_ARCH}
MLIBDIR=	64

COPTS+=		-mabi=64
CPUFLAGS+=	-mabi=64
LDADD+=		-mabi=64
LDFLAGS+=	-mabi=64
MKDEPFLAGS+=	-mabi=64

.include "${.PARSEDIR}/../../Makefile.compat"
