#	$NetBSD: Makefile,v 1.4 1996/08/30 20:13:39 thorpej Exp $

PROG =	rpc.sprayd
SRCS =	sprayd.c
MAN =	rpc.sprayd.8
MLINKS=	rpc.sprayd.8 sprayd.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

.include <bsd.prog.mk>

