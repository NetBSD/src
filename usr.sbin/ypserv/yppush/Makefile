#	$NetBSD: Makefile,v 1.2 1997/05/06 20:46:46 gwr Exp $

PROG=	yppush
SRCS=	localhostname.c yppush.c yplib_host.c yppush_svc.c yppush_proc.c \
	ypdb.c yppush_err.c
MAN=	yppush.8
.PATH.c: ${.CURDIR}/../common ${.CURDIR}/../ypserv
CFLAGS+=-I${.CURDIR}/../common

.include <bsd.prog.mk>
