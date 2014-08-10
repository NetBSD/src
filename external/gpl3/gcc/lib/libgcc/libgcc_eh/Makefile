#	$NetBSD: Makefile,v 1.9.2.1 2014/08/10 07:10:49 tls Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

LIB=		gcc_eh

.include "../../../Makefile.gcc_path"
.include <bsd.own.mk>

.if ${MKGCC} != "no"

.cc: # disable .cc->NULL transform

SRCS+=		${LIB2_EH} ${LIB2_EHASM} empty.c
COPTS.c-typeck.c=	-Wno-stack-protector
COPTS.unwind-dw2.c=	-Wno-stack-protector

MKPIC:=	no

.include <bsd.lib.mk>

.else
.include <bsd.prog.mk> # do nothing
.endif
