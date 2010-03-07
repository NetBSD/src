#	$NetBSD: Makefile,v 1.7 2010/03/07 16:27:18 plunky Exp $

WARNS?=	2

.include <bsd.init.mk>

.PATH:	${PCC_DIST}/cc/cpp \
	${PCC_DIST}/mip

#
# We build cpp(1) as pcpp(1) to avoid confusion with GCC
#

PROG=	pcpp

SRCS=	cpy.y
SRCS+=	cpp.c token.c

MAN=	pcpp.1

pcpp.1:	cpp.1
	${TOOL_SED} -e "s,Nm cpp,Nm pcpp,"	\
		    -e "s,Dt CPP,Dt PCPP,"	\
		    ${.ALLSRC} > ${.TARGET}

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
