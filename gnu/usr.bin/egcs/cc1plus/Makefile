#	$NetBSD: Makefile,v 1.7 1998/08/17 02:25:46 tv Exp $


PROG=		cc1plus
SRCS=		c-common.o c-pragma.o \
		call.c decl.c errfn.c expr.c pt.c sig.c typeck2.c class.c \
		decl2.c error.c lex.c parse.y ptree.c rtti.c spew.c typeck.c \
		cvt.c except.c friend.c init.c method.c search.c semantics.c \
		tree.c xref.c repo.c
CLEANFILES+=	parse-h
YHEADER=1

lex.o pt.o spew.o: parse-h
parse-h: parse.h
	grep '^#define[         ]*YYEMPTY' parse.c >>parse.h
	@touch parse-h

# Needs to be before dist/gcc or tree.c comes from the wrong place.
.PATH: ${.CURDIR}/../../../dist/gcc/cp

.include "../Makefile.cc1"

CPPFLAGS+=	-I. -I${DIST}/cp
