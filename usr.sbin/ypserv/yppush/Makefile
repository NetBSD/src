#	$NetBSD: Makefile,v 1.3 1997/05/08 21:12:55 gwr Exp $

PROG=	yppush
SRCS=	localhostname.c yppush.c yplib_host.c yppush_svc.c yppush_proc.c \
	ypdb.c yppush_err.c
MAN=	yppush.8
.PATH:	${.CURDIR}/../common ${.CURDIR}/../ypserv
CFLAGS+=-I${.CURDIR}/../common

.include <bsd.prog.mk>
