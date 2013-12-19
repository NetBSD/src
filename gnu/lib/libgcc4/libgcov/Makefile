#	$NetBSD: Makefile,v 1.3.8.1 2013/12/19 01:16:13 matt Exp $

REQUIRETOOLS=	yes
NOPIC=		# defined
NOPROFILE=	# defined
NOLINT=		# defined

LIB=		gcov

.include <bsd.own.mk>
.include "../../Makefile.inc"

.if exists(${.CURDIR}/arch/${GCC_MACHINE_ARCH}/defs.mk) && ${MKGCC} != "no"

.include "${.CURDIR}/arch/${GCC_MACHINE_ARCH}/defs.mk"

LIBGCOVSRCS=	${G_LIBGCOV:=.c}

CPPFLAGS+=	-I${.CURDIR}/arch/${GCC_MACHINE_ARCH}
CPPFLAGS+=	-Wno-stack-protector

DPSRCS+=	${.CURDIR}/arch/${GCC_MACHINE_ARCH}/defs.mk
SRCS+=		${LIBGCOVSRCS}

CLEANFILES+=	${LIBGCOVSRCS}

${LIBGCOVSRCS}: ${.CURDIR}/Makefile
	${_MKTARGET_CREATE}
	printf '#define L${.PREFIX}\n#include <libgcov.c>\n' >${.TARGET}

.include <bsd.lib.mk>
.else
.include <bsd.prog.mk> # do nothing
.endif
