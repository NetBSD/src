#	$NetBSD: Makefile,v 1.8 1997/10/17 06:51:04 mikel Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	apropos
SRCS=	apropos.c config.c
.PATH:	${.CURDIR}/../man
CPPFLAGS+=-I${.CURDIR}/../man

.include <bsd.prog.mk>
