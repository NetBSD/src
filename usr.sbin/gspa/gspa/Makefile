#	$NetBSD: Makefile,v 1.8 2001/12/12 01:48:50 tv Exp $

PROG=	gspa
CPPFLAGS+=-I. -I${.CURDIR}
HDRS=	gsp_ass.h gsp_code.h
SRCS=	gspa.c gsp_out.c gsp_sym.c gsp_lex.c gsp_act.c gsp_eval.c \
	gsp_inst.c gsp_pseu.c gsp_gram.y
NOMAN=	# defined
YHEADER=1

gsp_eval.o gsp_lex.o gspa.o: gsp_gram.h

.include <bsd.prog.mk>
