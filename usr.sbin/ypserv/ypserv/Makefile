#	$NetBSD: Makefile,v 1.8 1997/10/25 06:58:59 lukem Exp $

PROG=	ypserv
SRCS=	acl.c gram.c scan.c ypserv.c ypserv_proc.c ypserv_db.c ypserv_xdr.c
MAN=	ypserv.acl.5 securenet.5 ypserv.8
CPPFLAGS+=-DYY_NO_UNPUT -DOPTIMIZE_DB -I.

CLEANFILES+= gram.c scan.c y.tab.h

.include <bsd.prog.mk>
