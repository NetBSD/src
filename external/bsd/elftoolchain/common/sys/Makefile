# $NetBSD: Makefile,v 1.2 2024/03/27 21:55:35 christos Exp $

.include <bsd.own.mk>

TOP=	${.CURDIR}/../../dist
SRCDIR=	${TOP}/common/sys

.PATH:	${SRCDIR}

INCS=		elfdefinitions.h
INCSDIR=	/usr/include/sys

elfdefinitions.h:	elfdefinitions.m4 elfconstants.m4
	${_MKTARGET_CREATE}
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
