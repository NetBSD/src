#	$NetBSD: Makefile,v 1.27 2011/05/25 15:24:07 joerg Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist

PROG=		bc
SRCS=		bc.y execute.c global.c load.c main.c \
		number.c scan.l storage.c util.c
CPPFLAGS+=	-D_POSIX_SOURCE -funsigned-char -I. -I${.CURDIR} -I${DIST}/bc/bc -I${DIST}/bc/h
LFLAGS+=	-I8
LDADD+=		-ll -ledit -lterminfo
DPADD+=		${LIBL} ${LIBEDIT} ${LIBTERMINFO}
YHEADER=1

WARNS?=		0	# pointer sign issues

TEXINFO=	bc.texi

.PATH: ${DIST}/bc/bc ${DIST}/bc/lib ${DIST}/bc/doc

.include <bsd.prog.mk>
