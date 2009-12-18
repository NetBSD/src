#	$NetBSD: Makefile,v 1.8 2009/12/18 13:57:10 uebayasi Exp $

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

.include "../Makefile.srcs"

# New style.
TMPLSRCS=	${TMPLSRCS.libgcc_eh.a}
TMPLFPSRCS=	${TMPLFPSRCS.libgcc_eh.a}
TMPLASMSRCS=	${TMPLASMSRCS.libgcc_eh.a}
SRCS=		${SRCS.libgcc_eh.a}

.include "../Makefile.wrapper"

.endif	# defined(MKNATIVE_LIBGCC_NEW)

.include <bsd.lib.mk>

.else
.include <bsd.prog.mk> # do nothing
.endif
