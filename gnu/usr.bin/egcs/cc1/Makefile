#	$NetBSD: Makefile,v 1.6 1998/10/14 20:46:36 tv Exp $

PROG=		cc1
SRCS=		${C_OBJC_SRCS} c-parse.y c-lang.c

.NOPATH: c-parse.c c-parse.h
.BEGIN:
	@rm -f ${DIST}/c-parse.c ${DIST}/c-parse.h

.include "../Makefile.cc1"
