#	$NetBSD: Makefile,v 1.8.42.1 2018/10/20 06:58:18 pgoyette Exp $
# Build a tiny limited gzip (i.e. for tiny boot media)

SRCDIR=         ${.CURDIR}/../../../usr.bin/gzip

PROG=		gzip
NOMAN=		# defined

CPPFLAGS+=	-DSMALL
CPPFLAGS+=	-DNO_BZIP2_SUPPORT
CPPFLAGS+=	-DNO_COMPRESS_SUPPORT
CPPFLAGS+=	-DNO_PACK_SUPPORT
CPPFLAGS+=	${"${USE_XZ_SETS:Uno}"!="no":?:-DNO_XZ_SUPPORT}

# for crunched binaries this does not take effect, also check the
# LIBS entry in the lists file
DPADD=		${LIBZ} ${LIBLZMA}
LDADD=		-lz -llzma

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
