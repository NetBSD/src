#	$NetBSD: Makefile,v 1.1.1.1 1996/08/09 10:15:01 thorpej Exp $

PROG=	yppush
SRCS=	localhostname.c yppush.c yplib_host.c yppush_svc.c yppush_proc.c \
	ypdb.c yppush_err.c
MAN=	yppush.8
.PATH:	${.CURDIR}/../common ${.CURDIR}/../ypserv
CFLAGS+=-I${.CURDIR}/../common

.include <bsd.prog.mk>
