#	$NetBSD: Makefile,v 1.4 1997/07/18 21:57:11 thorpej Exp $

PROG=	yppush
SRCS=	localhostname.c yppush.c yplib_host.c yppush_svc.c yppush_proc.c \
	ypdb.c yppush_err.c
MAN=	yppush.8
.PATH:	${.CURDIR}/../common ${.CURDIR}/../ypserv
CFLAGS+=-I${.CURDIR}/../common
WARNS=1

.include <bsd.prog.mk>
