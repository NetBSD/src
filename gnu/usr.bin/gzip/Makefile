#	$NetBSD: Makefile,v 1.26 1999/02/02 02:04:40 thorpej Exp $

.include <bsd.own.mk>

PROG=	gzip
SRCS=	gzip.c zip.c deflate.c trees.c bits.c unzip.c inflate.c util.c \
	crypt.c lzw.c unlzw.c unlzh.c unpack.c getopt.c
MAN=	gzexe.1 gzip.1 zdiff.1 zforce.1 zgrep.1 zmore.1 znew.1
CPPFLAGS+=-DSTDC_HEADERS=1 -DHAVE_UNISTD_H=1 -DDIRENT=1 -Dunix

.if (${MACHINE_ARCH} == "m68k" || ${MACHINE_ARCH} == "i386")
SRCS+=	match.S
CPPFLAGS+=-DASMV
MATCHCPPFLAGS=-D__NetBSD__

.if (${OBJECT_FMT} == "ELF")
MATCHCPPFLAGS+=-DNO_UNDERLINE
.endif

.endif	# m68k || i386

LDSTATIC?= -static
LDADD+=	-lgnumalloc
DPADD+=	${LIBGNUMALLOC}

MLINKS+= gzip.1 gunzip.1 gzip.1 gzcat.1 gzip.1 zcat.1
MLINKS+= zdiff.1 zcmp.1
#MLINKS+= gzip.1 uncompress.1
MLINKS+= zgrep.1 zegrep.1 zgrep.1 zfgrep.1
LINKS+= ${BINDIR}/gzip ${BINDIR}/gunzip
LINKS+= ${BINDIR}/gzip ${BINDIR}/gzcat
LINKS+= ${BINDIR}/gzip ${BINDIR}/zcat
LINKS+=	${BINDIR}/zdiff ${BINDIR}/zcmp
#LINKS+= ${BINDIR}/gzip ${BINDIR}/uncompress
LINKS+=	${BINDIR}/zgrep ${BINDIR}/zegrep
LINKS+=	${BINDIR}/zgrep ${BINDIR}/zfgrep

FILES=zforce gzexe znew zmore zdiff zgrep 
FILESMODE=${BINMODE}

.if (${MACHINE_ARCH} == "m68k" || ${MACHINE_ARCH} == "i386")
match.o: ${.CURDIR}/match.S
	$(CPP) ${MATCHCPPFLAGS} ${.CURDIR}/match.S >_match.s
	$(CC) -c _match.s
	mv _match.o match.o
	rm -f _match.s
.endif

.include <bsd.prog.mk>
