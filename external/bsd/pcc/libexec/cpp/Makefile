#	$NetBSD: Makefile,v 1.4 2010/01/20 11:45:55 plunky Exp $

.include <bsd.init.mk>

.PATH:	${PCC_DIST}/cc/cpp \
	${PCC_DIST}/mip

PROG=	cpp

SRCS=	cpy.y
SRCS+=	cpp.c token.c

MAN=	cpp.1

CPPFLAGS+=	-DCPP_DEBUG
CPPFLAGS+=	-I${.OBJDIR}
CPPFLAGS+=	-I${PCC_DIST}/mip
CPPFLAGS+=	-I${PCC_DIST}/cc/cpp

# generate cpy.h
YHEADER=

# some files include y.tab.h instead
DPSRCS=		y.tab.h

y.tab.h:	cpy.h
	${HOST_LN} -f ${.ALLSRC} ${.TARGET}

CLEANFILES+=	y.tab.h

.include <bsd.prog.mk>
