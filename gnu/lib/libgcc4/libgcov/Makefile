#	$NetBSD: Makefile,v 1.7 2009/12/09 17:30:14 uebayasi Exp $

REQUIRETOOLS=	yes
NOPIC=		# defined
NOPROFILE=	# defined
NOLINT=		# defined

LIB=		gcov

.include <bsd.own.mk>

.if !defined(MKNATIVE_LIBGCC_NEW) && ${MKGCC} != "no"

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

LIBGCOVSRCS=	${G_LIBGCOV:=.c}

CPPFLAGS+=	-I${.CURDIR}/arch/${MACHINE_ARCH}
CPPFLAGS+=	-Wno-stack-protector

.if !defined(MKNATIVE_LIBGCC_NEW)

DPSRCS+=	${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk
SRCS+=		${LIBGCOVSRCS}

.else	# defined(MKNATIVE_LIBGCC_NEW)

# New style.
SRCS=		${SRCS.libgcov.a}
TMPLSRCS=	${TMPLSRCS.libgcov.a}
TMPLFPSRCS=	${TMPLFPSRCS.libgcov.a}
TMPLASMSRCS=	${TMPLASMSRCS.libgcov.a}

.endif	# defined(MKNATIVE_LIBGCC_NEW)

.if !defined(MKNATIVE_LIBGCC_NEW)

CLEANFILES+=	${LIBGCOVSRCS}

${LIBGCOVSRCS}: ${.CURDIR}/Makefile
	${_MKTARGET_CREATE}
	printf '#define L${.PREFIX}\n#include <libgcov.c>\n' >${.TARGET}

.endif	# !defined(MKNATIVE_LIBGCC_NEW)

.include <bsd.lib.mk>
.else
.include <bsd.prog.mk> # do nothing
.endif
