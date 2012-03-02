#	$NetBSD: Makefile,v 1.1.2.2 2012/03/02 18:27:55 riz Exp $

.include <bsd.init.mk>

PROG=	xmlwf

.PATH:	${EXPATSRCDIR}/xmlwf ${EXPATSRCDIR}/doc

SRCS=	codepage.c unixfilemap.c xmlfile.c xmlwf.c
CPPFLAGS+=-DHAVE_UNISTD_H

.include <bsd.prog.mk>
