#	$NetBSD: Makefile,v 1.11 2016/02/09 20:40:45 plunky Exp $

.include <bsd.init.mk>

.PATH:	${PCC_DIST}/cc/cpp

#
# We build cpp(1) as pcpp(1) to avoid confusion with GCC
#

PROG=	pcpp

SRCS+=	cpc.c cpp.c token.c

MAN=	pcpp.1

pcpp.1:	cpp.1
	${TOOL_SED} -e "s,Nm cpp,Nm pcpp,"	\
		    -e "s,Dt CPP,Dt PCPP,"	\
		    ${.ALLSRC} > ${.TARGET}

CPPFLAGS+=	-I${.OBJDIR}
CPPFLAGS+=	-I${PCC_DIST}/mip
CPPFLAGS+=	-I${PCC_DIST}/cc/cpp

COPTS.cpp.c+=	${${ACTIVE_CC} == "gcc" :? -Wno-error=cast-qual :}

CLEANFILES+=	pcpp.1

.include <bsd.prog.mk>
