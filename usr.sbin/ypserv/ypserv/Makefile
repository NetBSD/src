#	$NetBSD: Makefile,v 1.9 1998/04/09 00:32:41 tv Exp $

PROG=	ypserv
SRCS=	acl.c gram.y scan.l ypserv.c ypserv_proc.c ypserv_db.c ypserv_xdr.c
MAN=	ypserv.acl.5 securenet.5 ypserv.8
CPPFLAGS+=-DYY_NO_UNPUT -DOPTIMIZE_DB -I.
YHEADER=1

.include <bsd.prog.mk>
