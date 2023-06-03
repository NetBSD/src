#	$NetBSD: Makefile,v 1.7 2023/06/03 09:09:24 lukem Exp $

PROG=	yppush
SRCS=	yppush.c yppush_err.c yppush_proc.c yppush_svc.c
MAN=	yppush.8

COPTS.yppush.c+=	${CC_WNO_CAST_FUNCTION_TYPE}
COPTS.yppush_svc.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
