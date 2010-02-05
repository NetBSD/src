#	$NetBSD: Makefile,v 1.6 2010/02/05 08:37:48 plunky Exp $

WARNS?=	2

.include <bsd.init.mk>

.PATH:	${PCC_DIST}/cc/cpp \
	${PCC_DIST}/mip

PROG=	cpp

SRCS=	cpy.y
SRCS+=	cpp.c token.c

# avoid conflicts with the GCC cpp.1 page
MAN=	pcpp.1

pcpp.1:	cpp.1
	cp ${.ALLSRC} ${.TARGET}

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

CLEANFILES+=	pcpp.1 y.tab.h

.include <bsd.prog.mk>
