#	$NetBSD: Makefile,v 1.3 2010/02/05 08:37:48 plunky Exp $

WARNS?=	2

.include <bsd.init.mk>

.PATH: ${PCC_DIST}/cc/cc

#
# we build PCC cc(1) as pcc(1) to avoid conflicts with GCC
#

PROG=	pcc
SRCS=	cc.c
MAN=	pcc.1

CPPFLAGS+=	-I${PCC_DIST}/mip
CPPFLAGS+=	-I${PCC_DIST}/os/${TARGOS}

pcc.1:	cc.1
	${TOOL_SED} -e "s,Nm cc,Nm pcc," -e "s,Dt CC,Dt PCC," ${.ALLSRC} > ${.TARGET}

CLEANFILES+=	pcc.1

.include <bsd.prog.mk>
