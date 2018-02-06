#	$NetBSD: bsd.64.mk,v 1.12 2018/02/06 10:00:00 mrg Exp $

.if ${MACHINE_ARCH} == "mips64eb"
LD+=		-m elf64btsmip
.else
LD+=		-m elf64ltsmip
.endif
.ifndef MLIBDIR
MLIBDIR=	64

# GCC 5/6 libgomp for n64 needs files we don't generate yet.
NO_LIBGOMP=	1

COPTS+=		-mabi=64
CPUFLAGS+=	-mabi=64
LDADD+=		-mabi=64
LDFLAGS+=	-mabi=64
MKDEPFLAGS+=	-mabi=64
.endif

.include "${.PARSEDIR}/../../Makefile.compat"
