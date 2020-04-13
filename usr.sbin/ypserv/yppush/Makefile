#	$NetBSD: Makefile,v 1.5.110.1 2020/04/13 08:06:07 martin Exp $

PROG=	yppush
SRCS=	yppush.c yppush_err.c yppush_proc.c yppush_svc.c
MAN=	yppush.8

COPTS.yppush.c+=	${GCC_NO_CAST_FUNCTION_TYPE}
COPTS.yppush_svc.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
