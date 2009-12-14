#	$NetBSD: bsd.64.mk,v 1.2 2009/12/14 13:43:59 uebayasi Exp $

LD+=		-m elf64btsmip	# XXX endian
MLIBDIR=	64

COPTS+=		-mabi=64
CPUFLAGS+=	-mabi=64
LDADD+=		-mabi=64
LDFLAGS+=	-mabi=64
MKDEPFLAGS+=	-mabi=64

.include "${NETBSDSRCDIR}/compat/Makefile.compat"
