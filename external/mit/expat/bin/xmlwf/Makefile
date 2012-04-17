#	$NetBSD: Makefile,v 1.1.4.2 2012/04/17 00:04:38 yamt Exp $

.include <bsd.init.mk>

PROG=	xmlwf

.PATH:	${EXPATSRCDIR}/xmlwf ${EXPATSRCDIR}/doc

SRCS=	codepage.c unixfilemap.c xmlfile.c xmlwf.c
CPPFLAGS+=-DHAVE_UNISTD_H

.include <bsd.prog.mk>
