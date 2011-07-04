#	$NetBSD: bsd.64.mk,v 1.6 2011/07/04 12:00:49 mrg Exp $

.if ${MACHINE_ARCH} == "mips64eb"
LD+=		-m elf64btsmip
.else
LD+=		-m elf64ltsmip
.endif
MLIBDIR=	64

COPTS+=		-mabi=64
CPUFLAGS+=	-mabi=64
LDADD+=		-mabi=64
LDFLAGS+=	-mabi=64
MKDEPFLAGS+=	-mabi=64

LIBMPFR_MACHINE_ARCH=	mipseb
LIBGMP_MACHINE_ARCH=	mipseb

.include "${.PARSEDIR}/../../Makefile.compat"
