# $NetBSD: Makefile,v 1.1.1.1 1996/09/12 20:24:00 gwr Exp $
# Small zcat (i.e. for install media)
#
# Note: gzio.c is compiled here with -DNO_DEFLATE
# to eliminate references to large parts of libz.a

PROG=	zcat
NOMAN=
SRCS=	zcat.c gzio.c

SRCDIR= ${.CURDIR}/../../../lib/libz
CFLAGS+= -D_ZLIB_PRIVATE -I${SRCDIR}
CFLAGS+= -DNO_DEFLATE 

DPADD+=	${DESTDIR}/usr/lib/libz.a
LDADD+=	-lz

all: ${PROG}

.include <bsd.prog.mk>
.PATH:  ${SRCDIR}

test: zcat
	echo 'hello, hello!' | gzip | ./zcat
