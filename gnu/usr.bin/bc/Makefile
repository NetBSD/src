#	$NetBSD: Makefile,v 1.22 2000/11/01 13:14:46 wiz Exp $

PROG=		bc
SRCS=		bc.y execute.c global.c load.c main.c \
		number.c scan.l storage.c util.c
CPPFLAGS+=	-D_POSIX_SOURCE -I. -I${.CURDIR} -I${DIST}/bc/bc -I${DIST}/bc/h
LFLAGS+=	-I8
LDADD+=		-ll -ledit -ltermcap
DPADD+=		${LIBL} ${LIBEDIT} ${LIBTERMCAP}
YHEADER=1

DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/bc/bc ${DIST}/bc/lib ${DIST}/bc/doc

.include <bsd.prog.mk>
