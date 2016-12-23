#	$NetBSD: Makefile,v 1.26 2016/12/23 10:19:57 mrg Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/17/94

PROG=	savecore
SRCS=	savecore.c zopen.c

MAN=	savecore.8

LDADD=	-lkvm -lutil -lz
DPADD=	${LIBKVM} ${LIBUTIL} ${LIBZ}

.include "../../compat/exec.mk"

.include <bsd.prog.mk>
