#	$NetBSD: Makefile,v 1.5 1997/10/17 07:08:51 mikel Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	whatis
SRCS=	whatis.c config.c
.PATH:	${.CURDIR}/../man
CPPFLAGS+=-I${.CURDIR}/../man

.include <bsd.prog.mk>
