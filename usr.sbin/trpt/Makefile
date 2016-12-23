#	@(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.11 2016/12/23 10:19:58 mrg Exp $

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
