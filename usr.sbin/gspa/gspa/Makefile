#	$NetBSD: Makefile,v 1.4 1997/10/25 06:58:03 lukem Exp $

PROG=	gspa
CPPFLAGS+=-I. -I${.CURDIR}
HDRS=	gsp_ass.h gsp_code.h
SRCS=	gspa.c gsp_out.c gsp_sym.c gsp_lex.c gsp_act.c gsp_eval.c \
	gsp_inst.c gsp_pseu.c gsp_gram.c
NOMAN=	noman
CLEANFILES+=gsp_gram.c y.tab.h

gsp_eval.o gsp_lex.o gspa.o: gsp_gram.c

.include <bsd.prog.mk>
