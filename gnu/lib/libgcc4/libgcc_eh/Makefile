#	$NetBSD: Makefile,v 1.3 2006/11/09 04:07:11 christos Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

LIB=		gcc_eh

.include <bsd.own.mk>

.if ${MKGCC} != "no"
.if exists(${.CURDIR}/../arch/${MACHINE_ARCH}.mk)
.include "${.CURDIR}/../arch/${MACHINE_ARCH}.mk"
.endif

.cc: # disable .cc->NULL transform

SRCS+=		${LIB2_EH}
COPTS.unwind-dw2.c = -Wno-stack-protector


.include <bsd.lib.mk>

.include "../Makefile.unwind_h"

.else
.include <bsd.prog.mk> # do nothing
.endif
