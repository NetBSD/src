#	$NetBSD: Makefile,v 1.1.1.1 1996/08/09 10:15:05 thorpej Exp $

PROG=	ypserv
SRCS=	acl.c gram.c scan.c ypserv.c ypserv_proc.c ypserv_db.c yplog.c \
	ypdb.c ypserv_xdr.c ypresp_xdr.c
MAN=	ypserv.acl.5 securenet.5 ypserv.8
.PATH:	${.CURDIR}/../common
CFLAGS+=-I. -I${.CURDIR}/../common

CLEANFILES+= gram.c scan.c

.include <bsd.prog.mk>
