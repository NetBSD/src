#	$NetBSD: Makefile,v 1.5 1998/04/03 02:20:29 tv Exp $

PROG=	cc1obj
SRCS=	c-common.c c-decl.c c-lex.c c-typeck.c objc-parse.y objc-act.c

# XXX needs to be before dist/gcc or tree.c comes from the
# wrong place..
.PATH: ${.CURDIR}/../../../dist/gcc/objc

.include "../Makefile.cc1"

CPPFLAGS+=	-I${DIST}/objc
