#	$NetBSD: Makefile,v 1.5 1998/04/09 00:32:33 tv Exp $


PROG=	cc1plus
SRCS=	c-common.c call.c class.c cvt.c decl.c decl2.c errfn.c error.c except.c \
	expr.c friend.c init.c lex.c method.c parse.y pt.c ptree.c \
	repo.c rtti.c search.c sig.c spew.c tree.c typeck.c typeck2.c \
	xref.c
YHEADER=1

lex.c pt.c spew.c: parse-h

parse-h: parse.h
	grep '^#define[         ]*YYEMPTY' parse.c >>parse.h
	@touch parse-h
CLEANFILES+= parse-h

# XXX needs to be before dist/gcc or tree.c comes from the
# wrong place..
.PATH: ${.CURDIR}/../../../dist/gcc/cp

.include "../Makefile.cc1"

CPPFLAGS+= -I. -I${DIST}/cp
