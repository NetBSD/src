# $NetBSD: Makefile,v 1.1 2021/04/06 19:40:19 jkoshy Exp $

.include <bsd.own.mk>

TOP=	${.CURDIR}/../dist
SRCDIR=	${TOP}/common

.PATH:	${SRCDIR}

SUBDIR=		sys

INCS=		elfdefinitions.h
INCSDIR=	/usr/include

.PHONY:	all clean clobber depend obj

all depend:

clean clobber:
	rm -f ${CLEANFILES}

cleandepend:
	rm -f ${.OBJDIR}/.depend

.include <bsd.inc.mk>
.include <bsd.subdir.mk>
