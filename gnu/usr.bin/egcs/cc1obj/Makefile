#	$NetBSD: Makefile,v 1.9 1998/10/14 20:46:36 tv Exp $

PROG=		cc1obj
SRCS=		${C_OBJC_SRCS} objc-parse.y objc-act.c
DPSRCS+=	c-parse.h

.NOPATH: c-parse.c c-parse.h objc-parse.c objc-parse.h
.BEGIN:
	@rm -f ${DIST}/c-parse.c ${DIST}/c-parse.h \
		${DIST}/objc/objc-parse.c ${DIST}/objc/objc-parse.h

.include "../Makefile.cc1"

.PATH:		${DIST}/objc
CPPFLAGS+=	-I${DIST}/objc
