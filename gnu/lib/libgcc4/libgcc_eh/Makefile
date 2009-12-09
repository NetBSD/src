#	$NetBSD: Makefile,v 1.7 2009/12/09 06:20:14 uebayasi Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

LIB=		gcc_eh

.include <bsd.own.mk>

.if ${MKGCC} != "no"

.cc: # disable .cc->NULL transform

.if !defined(MKNATIVE_LIBGCC_NEW)

SRCS+=		${LIB2_EH}
COPTS.unwind-dw2.c = -Wno-stack-protector

.else	# defined(MKNATIVE_LIBGCC_NEW)

# New style.
TMPLSRCS=	${TMPLSRCS.libgcc_eh.a}
TMPLFPSRCS=	${TMPLFPSRCS.libgcc_eh.a}
TMPLASMSRCS=	${TMPLASMSRCS.libgcc_eh.a}
SRCS=		${SRCS.libgcc_eh.a}

.endif	# defined(MKNATIVE_LIBGCC_NEW)

.include <bsd.lib.mk>

.else
.include <bsd.prog.mk> # do nothing
.endif
