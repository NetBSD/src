#	$NetBSD: Makefile,v 1.2 2004/03/08 14:47:27 skrll Exp $

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

.else
.include <bsd.prog.mk> # do nothing
.endif
