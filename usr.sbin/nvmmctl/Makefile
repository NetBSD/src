#	$NetBSD: Makefile,v 1.1 2019/10/28 13:04:18 maxv Exp $

.include <bsd.own.mk>

MAN=	nvmmctl.8

.if (${MACHINE_ARCH} == "x86_64")
PROG=	nvmmctl
SRCS=	nvmmctl.c

CPPFLAGS+=-D_KERNTYPES
LDADD+=-lutil -lnvmm
DPADD+=${LIBUTIL} ${LIBNVMM}
.endif

.include <bsd.prog.mk>
