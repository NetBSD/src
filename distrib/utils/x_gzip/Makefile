#	$NetBSD: Makefile,v 1.12 2022/07/10 04:01:07 mrg Exp $
# Build a tiny limited gzip (i.e. for tiny boot media)

.include <bsd.own.mk>

SRCDIR=         ${.CURDIR}/../../../usr.bin/gzip

PROG=		gzip
NOMAN=		# defined

CPPFLAGS+=	-DSMALL
CPPFLAGS+=	-DNO_BZIP2_SUPPORT
CPPFLAGS+=	-DNO_COMPRESS_SUPPORT
CPPFLAGS+=	-DNO_PACK_SUPPORT
CPPFLAGS+=	-DNO_LZ_SUPPORT

# for crunched binaries this does not take effect, also check the
# LIBS entry in the lists file
DPADD=		${LIBZ}
LDADD=		-lz

.if ${USE_XZ_SETS:Uno} != "no"
DPADD+=		${LIBLZMA}
LDADD+=		-llzma
.else
CPPFLAGS+=	-DNO_XZ_SUPPORT
.endif

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
