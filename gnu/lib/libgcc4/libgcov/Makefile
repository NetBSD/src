#	$NetBSD: Makefile,v 1.5 2009/11/30 13:24:32 uebayasi Exp $

REQUIRETOOLS=	yes
NOPIC=		# defined
NOPROFILE=	# defined
NOLINT=		# defined

LIB=		gcov

.include <bsd.own.mk>

.if exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk) && ${MKGCC} != "no"

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

LIBGCOVSRCS=	${G_LIBGCOV:=.c}

CPPFLAGS+=	-I${.CURDIR}/arch/${MACHINE_ARCH}
CPPFLAGS+=	-Wno-stack-protector

.if !exists(${.CURDIR}/../arch/${MACHINE_ARCH}/libs.lib${LIB}.srcs.mk)

DPSRCS+=	${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk
SRCS+=		${LIBGCOVSRCS}

.else	# exists(${.CURDIR}/../arch/${MACHINE_ARCH}/libs.lib${LIB}.srcs.mk)

# New style.
SRCS=		${SRCS.libgcov.a}
TMPLSRCS=	${TMPLSRCS.libgcov.a}
TMPLFPSRCS=	${TMPLFPSRCS.libgcov.a}
TMPLASMSRCS=	${TMPLASMSRCS.libgcov.a}

.endif	# exists(${.CURDIR}/../arch/${MACHINE_ARCH}/libs.lib${LIB}.srcs.mk)

.if !exists(${.CURDIR}/../arch/${MACHINE_ARCH}/libs.lib${LIB}.srcs.mk)

CLEANFILES+=	${LIBGCOVSRCS}

${LIBGCOVSRCS}: ${.CURDIR}/Makefile
	${_MKTARGET_CREATE}
	printf '#define L${.PREFIX}\n#include <libgcov.c>\n' >${.TARGET}

.endif	# !exists(${.CURDIR}/../arch/${MACHINE_ARCH}/libs.lib${LIB}.srcs.mk)

.include <bsd.lib.mk>
.else
.include <bsd.prog.mk> # do nothing
.endif
