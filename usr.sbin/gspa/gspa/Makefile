#	$NetBSD: Makefile,v 1.6 1998/07/10 16:37:56 rvb Exp $

PROG=	gspa
CPPFLAGS+=-I. -I${.CURDIR}
HDRS=	gsp_ass.h gsp_code.h
SRCS=	gspa.c gsp_out.c gsp_sym.c gsp_lex.c gsp_act.c gsp_eval.c \
	gsp_inst.c gsp_pseu.c gsp_gram.y
NOMAN=	noman
YHEADER=1

gsp_eval.o gsp_lex.o gspa.o: gsp_gram.h

.include <bsd.prog.mk>
