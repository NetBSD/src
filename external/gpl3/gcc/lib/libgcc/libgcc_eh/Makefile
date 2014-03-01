#	$NetBSD: Makefile,v 1.7 2014/03/01 10:00:36 mrg Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

LIB=		gcc_eh

.include <bsd.own.mk>

.if ${MKGCC} != "no"

.cc: # disable .cc->NULL transform

SRCS+=		${LIB2_EH} ${LIB2_EHASM}
COPTS.unwind-dw2.c = -Wno-stack-protector

MKPIC:=	no

.include <bsd.lib.mk>

.else
.include <bsd.prog.mk> # do nothing
.endif
