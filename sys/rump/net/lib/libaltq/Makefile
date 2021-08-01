#	$NetBSD: Makefile,v 1.1.4.2 2021/08/01 22:42:43 thorpej Exp $
#


.PATH:	${.CURDIR}/../../../../altq

LIB=	rumpnet_altq
COMMENT=ALTQ support

SRCS+=	altq_subr.c altq_conf.c
SRCS+=	altq_cbq.c altq_rmclass.c
SRCS+=	altq_component.c

.include <bsd.init.mk>

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
