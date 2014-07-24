#	$NetBSD: Makefile,v 1.10 2014/07/24 20:20:49 plunky Exp $

.include <bsd.init.mk>

.PATH:	${PCC_DIST}/cc/cpp

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

CPPFLAGS+=	-I${.OBJDIR}
CPPFLAGS+=	-I${PCC_DIST}/mip
CPPFLAGS+=	-I${PCC_DIST}/cc/cpp

# generate cpy.h
YHEADER=

CLEANFILES+=	pcpp.1

.include <bsd.prog.mk>
