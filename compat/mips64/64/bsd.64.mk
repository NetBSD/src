#	$NetBSD: bsd.64.mk,v 1.4 2009/12/19 04:11:32 christos Exp $

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

.include "${.PARSEDIR}/../../Makefile.compat"
