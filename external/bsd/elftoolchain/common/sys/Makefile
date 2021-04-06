# $NetBSD: Makefile,v 1.1 2021/04/06 19:40:19 jkoshy Exp $

.include <bsd.own.mk>

TOP=	${.CURDIR}/../../dist
SRCDIR=	${TOP}/common/sys

.PATH:	${SRCDIR}

INCS=		elfdefinitions.h
INCSDIR=	/usr/include/sys

elfdefinitions.h:	elfdefinitions.m4 elfconstants.m4
	${TOOL_M4} -I${SRCDIR} -D SRCDIR=${SRCDIR} ${M4FLAGS} \
		elfdefinitions.m4 > ${.TARGET}

.PHONY:	all clean clobber depend

all depend:	elfdefinitions.h

clean clobber:
	rm -f ${INCS}

cleandepend:
	rm -f ${.OBJDIR}/.depend

.include <bsd.inc.mk>
.include <bsd.obj.mk>
