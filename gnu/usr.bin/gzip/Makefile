#	$Id: Makefile,v 1.7 1993/10/15 23:14:16 jtc Exp $

PROG=	gzip
SRCS=	gzip.c zip.c deflate.c trees.c bits.c unzip.c inflate.c util.c \
	crypt.c lzw.c unlzw.c unlzh.c unpack.c getopt.c match.S
MAN1=	gzexe.0 gzip.0 zdiff.0 zforce.0 zmore.0 znew.0
CFLAGS+=-DASMV -DSTDC_HEADERS=1 -DHAVE_UNISTD_H=1 -DDIRENT=1

LDADD+=	-lgnumalloc
DPADD+=	/usr/lib/libgnumalloc.a

MLINKS= gzip.1 gunzip.1  gzip.1 zcat.1  gzip.1 gzcat.1
LINKS+= ${BINDIR}/gzip ${BINDIR}/gunzip
LINKS+= ${BINDIR}/gzip ${BINDIR}/gzcat
LINKS+= ${BINDIR}/gzip ${BINDIR}/zcat

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	   ${.CURDIR}/zforce ${.CURDIR}/gzexe ${.CURDIR}/znew \
	   ${.CURDIR}/zmore ${.CURDIR}/zdiff ${.CURDIR}/zgrep \
	   ${DESTDIR}${BINDIR}

match.o: ${.CURDIR}/match.S
	$(CPP) ${.CURDIR}/match.S >_match.s
	$(CC) -c _match.s
	mv _match.o match.o
	rm -f _match.s

.include <bsd.prog.mk>
