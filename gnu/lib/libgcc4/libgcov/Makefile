#	$NetBSD: Makefile,v 1.3 2008/10/06 14:20:12 tron Exp $

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

DPSRCS+=	${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk
SRCS+=		${LIBGCOVSRCS}

CLEANFILES+=	${LIBGCOVSRCS}

${LIBGCOVSRCS}: ${.CURDIR}/Makefile
	${_MKTARGET_CREATE}
	printf '#define L${.PREFIX}\n#include <libgcov.c>\n' >${.TARGET}

.include <bsd.lib.mk>
.else
.include <bsd.prog.mk> # do nothing
.endif
