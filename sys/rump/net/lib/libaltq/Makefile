#	$NetBSD: Makefile,v 1.1 2021/07/14 03:19:24 ozaki-r Exp $
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
