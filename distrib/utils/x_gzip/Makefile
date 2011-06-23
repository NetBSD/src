#	$NetBSD: Makefile,v 1.7.54.1 2011/06/23 14:18:24 cherry Exp $
# Build a tiny limited gzip (i.e. for tiny boot media)

SRCDIR=         ${.CURDIR}/../../../usr.bin/gzip

PROG=		gzip
NOMAN=		# defined

CPPFLAGS+=	-DSMALL
CPPFLAGS+=	-DNO_BZIP2_SUPPORT
CPPFLAGS+=	-DNO_COMPRESS_SUPPORT
CPPFLAGS+=	-DNO_PACK_SUPPORT
CPPFLAGS+=	-DNO_XZ_SUPPORT

DPADD=		${LIBZ}
LDADD=		-lz

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
