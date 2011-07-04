#	$NetBSD: bsd.o32.mk,v 1.6 2011/07/04 12:00:49 mrg Exp $

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

LIBMPFR_MACHINE_ARCH=	mipsel
LIBGMP_MACHINE_ARCH=	mipsel

.include "${.PARSEDIR}/../../Makefile.compat"
