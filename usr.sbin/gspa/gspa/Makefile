#	$NetBSD: Makefile,v 1.5 1998/04/09 00:32:40 tv Exp $

PROG=	gspa
CPPFLAGS+=-I. -I${.CURDIR}
HDRS=	gsp_ass.h gsp_code.h
SRCS=	gspa.c gsp_out.c gsp_sym.c gsp_lex.c gsp_act.c gsp_eval.c \
	gsp_inst.c gsp_pseu.c gsp_gram.y
NOMAN=	noman
YHEADER=1

gsp_eval.c gsp_lex.c gspa.c: gsp_gram.h

.include <bsd.prog.mk>
