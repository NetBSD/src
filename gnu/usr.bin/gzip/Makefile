#	$Id: Makefile,v 1.12.2.1 1994/10/17 21:48:51 cgd Exp $

PROG=	gzip
SRCS=	gzip.c zip.c deflate.c trees.c bits.c unzip.c inflate.c util.c \
	crypt.c lzw.c unlzw.c unlzh.c unpack.c getopt.c
MAN1=	gzexe.0 gzip.0 zdiff.0 zforce.0 zmore.0 znew.0
CFLAGS+=-DSTDC_HEADERS=1 -DHAVE_UNISTD_H=1 -DDIRENT=1

.if (${MACHINE_ARCH} == "m68k" || ${MACHINE_ARCH} == "i386")
SRCS+=	match.S
CFLAGS+=-DASMV
.endif

LDSTATIC= -static
LDADD+=	-lgnumalloc
DPADD+=	/usr/lib/libgnumalloc.a

MLINKS= gzip.1 gunzip.1 gzip.1 gzcat.1 gzip.1 zcat.1
#MLINKS+= gzip.1 uncompress.1
LINKS+= ${BINDIR}/gzip ${BINDIR}/gunzip
LINKS+= ${BINDIR}/gzip ${BINDIR}/gzcat
#LINKS+= ${BINDIR}/gzip ${BINDIR}/uncompress
LINKS+= ${BINDIR}/gzip ${BINDIR}/zcat

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	   ${.CURDIR}/zforce ${.CURDIR}/gzexe ${.CURDIR}/znew \
	   ${.CURDIR}/zmore ${.CURDIR}/zdiff ${.CURDIR}/zgrep \
	   ${DESTDIR}${BINDIR}

.if (${MACHINE_ARCH} == "m68k" || ${MACHINE_ARCH} == "i386")
match.o: ${.CURDIR}/match.S
	$(CPP) -D__NetBSD__ ${.CURDIR}/match.S >_match.s
	$(CC) -c _match.s
	mv _match.o match.o
	rm -f _match.s
.endif

.include <bsd.prog.mk>
