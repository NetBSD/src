#	$NetBSD: Makefile,v 1.7 1998/08/17 02:25:45 tv Exp $

PROG=		cc1obj
SRCS=		${C_OBJC_SRCS} objc-parse.y objc-act.c

.include "../Makefile.cc1"

.PATH:		${DIST}/objc
CPPFLAGS+=	-I${DIST}/objc
