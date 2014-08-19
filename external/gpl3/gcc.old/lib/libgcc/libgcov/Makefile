#	$NetBSD: Makefile,v 1.1.1.2.8.2 2014/08/19 23:58:07 tls Exp $

REQUIRETOOLS=	yes
NOPIC=		# defined
NOPROFILE=	# defined
NOLINT=		# defined

LIB=		gcov

.include "../../../Makefile.gcc_path"
.include <bsd.own.mk>
.include <bsd.init.mk>

.if ${MKGCC} != "no"

.include "${.CURDIR}/arch/${LIBGCC_MACHINE_ARCH}/defs.mk"

LIBGCOVSRCS=	${G_LIBGCOV:=.c}

CPPFLAGS+=	-I${.CURDIR}/arch/${LIBGCC_MACHINE_ARCH}
CPPFLAGS+=	-Wno-stack-protector

DPSRCS+=	${.CURDIR}/arch/${LIBGCC_MACHINE_ARCH}/defs.mk
SRCS+=		${LIBGCOVSRCS}

CLEANFILES+=	${LIBGCOVSRCS}

${LIBGCOVSRCS}: ${.CURDIR}/Makefile
	${_MKTARGET_CREATE}
	printf '#define L${.PREFIX}\n#include <libgcov.c>\n' >${.TARGET}

.include <bsd.lib.mk>
.else
.include <bsd.prog.mk> # do nothing
.endif
