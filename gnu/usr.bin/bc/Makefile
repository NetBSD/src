#	$NetBSD: Makefile,v 1.23 2001/01/08 04:53:59 phil Exp $

PROG=		bc
SRCS=		bc.y execute.c global.c load.c main.c \
		number.c scan.l storage.c util.c getopt.c getopt1.c
CPPFLAGS+=	-D_POSIX_SOURCE -funsigned-char -I. -I${.CURDIR} -I${DIST}/bc/bc -I${DIST}/bc/h
LFLAGS+=	-I8
LDADD+=		-ll -ledit -ltermcap
DPADD+=		${LIBL} ${LIBEDIT} ${LIBTERMCAP}
YHEADER=1

TEXINFO=	bc.texi

DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/bc/bc ${DIST}/bc/lib ${DIST}/bc/doc

.include <bsd.prog.mk>
