#	$NetBSD: Makefile,v 1.32 2003/01/27 09:07:13 wiz Exp $

.include <bsd.own.mk>		# for MKDYNAMICROOT & OBJECT_FMT definition

PROG=		gzip
SRCS=		gzip.c zip.c deflate.c trees.c bits.c unzip.c inflate.c util.c \
		crypt.c lzw.c unbzip2.c unlzw.c unlzh.c unpack.c getopt.c
MAN=		gzexe.1 gzip.1 zdiff.1 zforce.1 zgrep.1 zmore.1 znew.1
CPPFLAGS+=	-DSTDC_HEADERS=1 -DHAVE_UNISTD_H=1 -DDIRENT=1 -Dunix

.if (${MACHINE_ARCH} == "m68k" || ${MACHINE_ARCH} == "i386")
SRCS+=		match.S
CPPFLAGS+=	-DASMV

.if (${OBJECT_FMT} == "ELF")
AFLAGS+=	-DNO_UNDERLINE
.endif
.endif	# m68k || i386

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

DPADD+=		${LIBBZ2}
LDADD+=		-lbz2

MLINKS+=	gzip.1 gunzip.1 gzip.1 gzcat.1 gzip.1 zcat.1 \
		zdiff.1 zcmp.1 \
		zgrep.1 zegrep.1 zgrep.1 zfgrep.1
#MLINKS+=	gzip.1 uncompress.1
LINKS+=		${BINDIR}/gzip ${BINDIR}/gunzip \
		${BINDIR}/gzip ${BINDIR}/gzcat \
		${BINDIR}/gzip ${BINDIR}/zcat \
		${BINDIR}/zdiff ${BINDIR}/zcmp \
		${BINDIR}/zgrep ${BINDIR}/zegrep \
		${BINDIR}/zgrep ${BINDIR}/zfgrep \
#LINKS+=	${BINDIR}/gzip ${BINDIR}/uncompress

FILES=		zforce gzexe znew zmore zdiff zgrep 
FILESMODE=	${BINMODE}

TEXINFO=	gzip.info
INFOFLAGS=	-I${.CURDIR}

.include <bsd.info.mk>
.include <bsd.prog.mk>
