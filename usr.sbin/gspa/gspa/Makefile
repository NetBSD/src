#	$NetBSD: Makefile,v 1.11 2006/08/26 18:15:37 christos Exp $

PROG=	gspa
MAN=	gspa.8
CPPFLAGS+=-I. -I${.CURDIR}
HDRS=	gsp_ass.h gsp_code.h
SRCS=	gspa.c gsp_out.c gsp_sym.c gsp_lex.c gsp_act.c gsp_eval.c \
	gsp_inst.c gsp_pseu.c gsp_gram.y
YHEADER=1

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
