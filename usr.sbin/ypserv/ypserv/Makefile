#	$NetBSD: Makefile,v 1.5 1997/07/18 21:57:16 thorpej Exp $

PROG=	ypserv
SRCS=	acl.c gram.c scan.c ypserv.c ypserv_proc.c ypserv_db.c yplog.c \
	ypdb.c ypserv_xdr.c ypresp_xdr.c
MAN=	ypserv.acl.5 securenet.5 ypserv.8
.PATH:	${.CURDIR}/../common
CFLAGS+=-I. -I${.CURDIR}/../common
CPPFLAGS+=-DYY_NO_UNPUT
WARNS=1

CLEANFILES+= gram.c scan.c y.tab.h

.include <bsd.prog.mk>
