#	$NetBSD: Makefile,v 1.2 1996/12/22 16:57:40 veego Exp $

PROG=	ypserv
SRCS=	acl.c gram.c scan.c ypserv.c ypserv_proc.c ypserv_db.c yplog.c \
	ypdb.c ypserv_xdr.c ypresp_xdr.c
MAN=	ypserv.acl.5 securenet.5 ypserv.8
.PATH:	${.CURDIR}/../common
CFLAGS+=-I. -I${.CURDIR}/../common

CLEANFILES+= gram.c scan.c y.tab.h

.include <bsd.prog.mk>
