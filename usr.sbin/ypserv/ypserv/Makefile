#	$NetBSD: Makefile,v 1.6 1997/10/02 14:16:46 lukem Exp $

PROG=	ypserv
SRCS=	acl.c gram.c scan.c ypserv.c ypserv_proc.c ypserv_db.c ypserv_xdr.c
MAN=	ypserv.acl.5 securenet.5 ypserv.8
CFLAGS+=-I.
CPPFLAGS+=-DYY_NO_UNPUT

CLEANFILES+= gram.c scan.c y.tab.h

.include <bsd.prog.mk>
