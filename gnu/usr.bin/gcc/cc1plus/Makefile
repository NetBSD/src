#	$NetBSD: Makefile,v 1.5 1998/04/09 00:32:34 tv Exp $

PROG=	cc1plus
SRCS=	parse.y call.c decl.c errfn.c expr.c \
	pt.c sig.c typeck2.c class.c decl2.c \
	error.c gc.c lex.c ptree.c spew.c \
	typeck.c cvt.c edsel.c except.c init.c \
	method.c search.c tree.c xref.c repo.c
YHEADER=1

lex.c pt.c spew.c: parse-h

parse-h: parse.h
	grep '^#define[         ]*YYEMPTY' parse.c >>parse.h
	@touch parse-h
CLEANFILES+= parse-h

.include "../Makefile.cc1"
