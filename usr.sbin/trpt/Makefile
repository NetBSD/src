#	@(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.10.58.1 2017/01/07 08:57:01 pgoyette Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network client, effectively

PROG=	trpt
MAN=	trpt.8

DPADD=	${LIBKVM}
LDADD=	-lkvm

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

.include "../../compat/exec.mk"

.include <bsd.prog.mk>
