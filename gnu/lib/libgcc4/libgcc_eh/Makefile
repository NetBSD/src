#	$NetBSD: Makefile,v 1.5 2008/09/18 12:20:19 skrll Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

LIB=		gcc_eh

.include <bsd.own.mk>

.if ${MKGCC} != "no"

.cc: # disable .cc->NULL transform

SRCS+=		${LIB2_EH}
COPTS.unwind-dw2.c = -Wno-stack-protector

.include <bsd.lib.mk>

.else
.include <bsd.prog.mk> # do nothing
.endif
