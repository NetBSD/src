#	$NetBSD: Makefile,v 1.2 1996/03/05 01:02:05 jtc Exp $

PROG=	cc1plus

SRCS=	call.c decl.c errfn.c expr.c pt.c sig.c \
typeck2.c class.c decl2.c error.c gc.c lex.c \
parse.c ptree.c spew.c typeck.c cvt.c edsel.c \
except.c init.c method.c search.c tree.c xref.c \
repo.c 

lex.o pt.o spew.o: parse.h

CLEANFILES += parse.c parse.h
parse.h: parse.c
parse.c: parse.y
	$(YACC) -d $(.IMPSRC)
	grep '^#define[         ]*YYEMPTY' y.tab.c >>y.tab.h
	mv -f y.tab.c parse.c
	mv -f y.tab.h parse.h

.include <../Makefile.cc1>
