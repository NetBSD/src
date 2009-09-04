#	$NetBSD: Makefile,v 1.3 2009/09/04 00:50:06 gmcgarry Exp $

.include <bsd.own.mk>

# USE_FORT?= yes

DIST=	${NETBSDSRCDIR}/external/bsd/pcc/dist/pcc
SUBDIST=${DIST}/cc/cpp
.PATH:	${SUBDIST}

PROG=	cpp
SRCS=   cpy.y cpp.c token.c

MAN=    cpp.1

BINDIR= /usr/libexec

YFLAGS+=	-d
CPPFLAGS+=	-I${DIST}/cc/cpp -I${.CURDIR} -I${DIST} -I${DIST}/mip
CPPFLAGS+=	-I${.CURDIR}/../..
CPPFLAGS+=	-I${.OBJDIR}
CPPFLAGS+=	-DCPP_DEBUG

CLEANFILES+=	lex.yy.c y.tab.h cpp.cat1 scanner.c
CLEANFILES+=    cpp.cat1

# scanner.l expects to include y.tab.h
cpy.c: cpy.y
	${YACC} ${YFLAGS} -o ${.TARGET} ${.ALLSRC}
	mv cpy.h y.tab.h

.include <bsd.prog.mk>
