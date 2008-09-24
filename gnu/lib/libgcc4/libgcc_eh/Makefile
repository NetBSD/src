#	$NetBSD: Makefile,v 1.3.18.2 2008/09/24 16:35:07 wrstuden Exp $

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
