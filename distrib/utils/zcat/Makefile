# $NetBSD: Makefile,v 1.3 1997/03/24 22:21:23 christos Exp $
# Small zcat (i.e. for install media)
#
# Note: gzio.c is compiled here so that crunchgen will assume
# the same symbol space for zcat.c and gzio.c which is required
# so that the fake deflate functions in zcat.c will satisfy the
# references to those functions in gzio.c (yes, it's a hack).

PROG=	zcat
NOMAN=
SRCS=	zcat.c gzio.c

SRCDIR= ${.CURDIR}/../../../lib/libz
CFLAGS+= -D_ZLIB_PRIVATE -I${SRCDIR}

DPADD+=	${LIBZ}
LDADD+=	-lz

all: ${PROG}

.include <bsd.prog.mk>
.PATH:  ${SRCDIR}

test: zcat
	echo 'hello, hello!' | gzip | ./zcat
