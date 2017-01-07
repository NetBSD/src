#	$NetBSD: Makefile,v 1.25.84.1 2017/01/07 08:56:07 pgoyette Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/17/94

PROG=	savecore
SRCS=	savecore.c zopen.c

MAN=	savecore.8

LDADD=	-lkvm -lutil -lz
DPADD=	${LIBKVM} ${LIBUTIL} ${LIBZ}

.include "../../compat/exec.mk"

.include <bsd.prog.mk>
