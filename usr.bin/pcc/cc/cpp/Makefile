#	$NetBSD: Makefile,v 1.1 2007/09/21 21:10:55 abs Exp $

.include <bsd.own.mk>

# USE_FORT?= yes

DIST=	${NETBSDSRCDIR}/dist/pcc
SUBDIST=${DIST}/cc/cpp
.PATH:	${SUBDIST}

PROG=	cpp
SRCS=   cpy.y scanner.c cpp.c

BINDIR= /usr/libexec

YFLAGS+=	-d
CPPFLAGS+=	-I${DIST}/cc/cpp -I${.CURDIR}
CPPFLAGS+=	-DCPP_DEBUG

CLEANFILES+=	lex.yy.c y.tab.h cpp.cat1

# scanner.l expects to include y.tab.h
cpy.c: cpy.y
	${YACC} ${YFLAGS} -o ${.TARGET} ${.ALLSRC}
	mv cpy.h y.tab.h

.include <bsd.prog.mk>
