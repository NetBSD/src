#	$NetBSD: Makefile,v 1.3 1997/05/06 20:46:47 gwr Exp $

PROG=	ypserv
SRCS=	acl.c gram.c scan.c ypserv.c ypserv_proc.c ypserv_db.c yplog.c \
	ypdb.c ypserv_xdr.c ypresp_xdr.c
MAN=	ypserv.acl.5 securenet.5 ypserv.8
.PATH.c: ${.CURDIR}/../common
CFLAGS+=-I. -I${.CURDIR}/../common

CLEANFILES+= gram.c scan.c y.tab.h

.include <bsd.prog.mk>
