#	$NetBSD: Makefile,v 1.6 2019/10/13 07:28:22 mrg Exp $

PROG=	yppush
SRCS=	yppush.c yppush_err.c yppush_proc.c yppush_svc.c
MAN=	yppush.8

COPTS.yppush.c+=	${GCC_NO_CAST_FUNCTION_TYPE}
COPTS.yppush_svc.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
