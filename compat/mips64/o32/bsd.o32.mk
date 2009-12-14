#	$NetBSD: bsd.o32.mk,v 1.2 2009/12/14 13:43:59 uebayasi Exp $

LD+=		-m elf32btsmip	# XXX endian
MLIBDIR=	o32

COPTS+=		-mabi=32 -march=mips3
CPUFLAGS+=	-mabi=32 -march=mips3
LDADD+=		-mabi=32 -march=mips3
LDFLAGS+=	-mabi=32 -march=mips3
MKDEPFLAGS+=	-mabi=32 -march=mips3

.include "${NETBSDSRCDIR}/compat/Makefile.compat"
