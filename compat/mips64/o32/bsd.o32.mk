#	$NetBSD: bsd.o32.mk,v 1.5 2011/07/01 01:30:16 mrg Exp $

.if ${MACHINE_ARCH} == "mips64eb"
LD+=		-m elf32btsmip
.else
LD+=		-m elf32ltsmip
.endif
MLIBDIR=	o32

COPTS+=		-mabi=32 -march=mips3
CPUFLAGS+=	-mabi=32 -march=mips3
LDADD+=		-mabi=32 -march=mips3
LDFLAGS+=	-mabi=32 -march=mips3
MKDEPFLAGS+=	-mabi=32 -march=mips3

LIBMPFR_MACHINE_ARCH=	${MLIBDIR}
LIBGMP_MACHINE_ARCH=	${MLIBDIR}

.include "${.PARSEDIR}/../../Makefile.compat"
