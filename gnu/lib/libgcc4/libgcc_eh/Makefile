#	$NetBSD: Makefile,v 1.2 2006/06/23 17:27:08 mrg Exp $

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

.include <bsd.lib.mk>

.include "../Makefile.unwind_h"

.else
.include <bsd.prog.mk> # do nothing
.endif
