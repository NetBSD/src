#	$NetBSD: Makefile,v 1.5 1997/09/17 20:16:07 christos Exp $

WARNS?=	1
PROG =	rpc.sprayd
SRCS =	sprayd.c
MAN =	rpc.sprayd.8
MLINKS=	rpc.sprayd.8 sprayd.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

.include <bsd.prog.mk>

