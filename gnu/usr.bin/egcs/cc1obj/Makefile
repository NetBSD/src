#	$NetBSD: Makefile,v 1.8 1998/08/17 22:29:55 tv Exp $

PROG=		cc1obj
SRCS=		${C_OBJC_SRCS} objc-parse.y objc-act.c
DPSRCS+=	c-parse.h

.include "../Makefile.cc1"

.PATH:		${DIST}/objc
CPPFLAGS+=	-I${DIST}/objc
