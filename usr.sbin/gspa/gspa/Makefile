#	$NetBSD: Makefile,v 1.10 2003/08/01 17:04:07 lukem Exp $

PROG=	gspa
MAN=	gspa.8
CPPFLAGS+=-I. -I${.CURDIR}
HDRS=	gsp_ass.h gsp_code.h
SRCS=	gspa.c gsp_out.c gsp_sym.c gsp_lex.c gsp_act.c gsp_eval.c \
	gsp_inst.c gsp_pseu.c gsp_gram.y
YHEADER=1

.include <bsd.prog.mk>
