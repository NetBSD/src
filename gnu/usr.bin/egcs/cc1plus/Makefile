#	$NetBSD: Makefile,v 1.3 1998/04/01 21:13:24 tv Exp $


PROG=	cc1plus
SRCS=	call.c class.c cvt.c decl.c decl2.c errfn.c error.c except.c \
	expr.c friend.c init.c lex.c method.c parse.c pt.c ptree.c \
	repo.c rtti.c search.c sig.c spew.c tree.c typeck.c typeck2.c \
	xref.c

lex.o pt.o spew.o: parse.h

CLEANFILES+=	parse.c parse.h

parse.h: parse.c
parse.c: parse.y
	$(YACC) -d $(.IMPSRC)
	grep '^#define[         ]*YYEMPTY' y.tab.c >>y.tab.h
	mv -f y.tab.c parse.c
	mv -f y.tab.h parse.h

# XXX needs to be before dist/gcc or tree.c comes from the
# wrong place..
.PATH: $(.CURDIR)/../../../dist/gcc/cp

.include "../Makefile.cc1"

CPPFLAGS+= -I. -I$(DIST)/cp
