#	$NetBSD: Makefile,v 1.6 2009/11/30 13:07:38 uebayasi Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

LIB=		gcc_eh

.include <bsd.own.mk>

.if ${MKGCC} != "no"

.cc: # disable .cc->NULL transform

.if !exists(${.CURDIR}/../arch/${MACHINE_ARCH}/libs.lib${LIB}.srcs.mk)

SRCS+=		${LIB2_EH}
COPTS.unwind-dw2.c = -Wno-stack-protector

.else	# exists(${.CURDIR}/../arch/${MACHINE_ARCH}/libs.lib${LIB}.srcs.mk)

# New style.
TMPLSRCS=	${TMPLSRCS.libgcc_eh.a}
TMPLFPSRCS=	${TMPLFPSRCS.libgcc_eh.a}
TMPLASMSRCS=	${TMPLASMSRCS.libgcc_eh.a}
SRCS=		${SRCS.libgcc_eh.a}

.endif	# exists(${.CURDIR}/../arch/${MACHINE_ARCH}/libs.lib${LIB}.srcs.mk)

.include <bsd.lib.mk>

.else
.include <bsd.prog.mk> # do nothing
.endif
