#	$Id: Makefile,v 1.1 1994/07/30 19:30:41 chopps Exp $

SRCDIR=	${.CURDIR}/../../../../../gnu/usr.bin/gzip
PROG=	gzip
SRCS=	gzip.c zip.c deflate.c trees.c bits.c unzip.c inflate.c util.c \
	crypt.c lzw.c unlzw.c unlzh.c unpack.c getopt.c
NOMAN=noman
CFLAGS+=-DSTDC_HEADERS=1 -DHAVE_UNISTD_H=1 -DDIRENT=1

.if (${MACHINE_ARCH} == "m68k" || ${MACHINE_ARCH} == "i386")
SRCS+=	match.S
CFLAGS+=-DASMV
.endif

LDSTATIC= -static
LDADD+=	-lgnumalloc
DPADD+=	/usr/lib/libgnumalloc.a

.if (${MACHINE_ARCH} == "m68k" || ${MACHINE_ARCH} == "i386")
match.o: ${SRCDIR}/match.S
	$(CPP) -D__NetBSD__ ${SRCDIR}/match.S >_match.s
	$(CC) -c _match.s
	mv _match.o match.o
	rm -f _match.s
.endif

BINDIR= /usr/bin

.PATH:	${SRCDIR}

.include <bsd.prog.mk>
