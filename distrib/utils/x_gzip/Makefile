#	$NetBSD: Makefile,v 1.7 2004/03/28 04:10:43 mrg Exp $
# Build a tiny limited gzip (i.e. for tiny boot media)

SRCDIR=         ${.CURDIR}/../../../usr.bin/gzip

PROG=		gzip
NOMAN=		# defined

CPPFLAGS+=	-DSMALL -DNO_BZIP2_SUPPORT -DNO_COMPRESS_SUPPORT

DPADD=		${LIBZ}
LDADD=		-lz

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
