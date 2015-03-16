#	$NetBSD: Makefile,v 1.7.56.1 2015/03/16 21:21:08 bouyer Exp $
# Build a tiny limited gzip (i.e. for tiny boot media)

SRCDIR=         ${.CURDIR}/../../../usr.bin/gzip

PROG=		gzip
NOMAN=		# defined

CPPFLAGS+=	-DSMALL -DNO_BZIP2_SUPPORT -DNO_COMPRESS_SUPPORT -DNO_XZ_SUPPORT

DPADD=		${LIBZ}
LDADD=		-lz

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
