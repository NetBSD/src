#	$NetBSD: Makefile,v 1.2 2019/10/29 08:13:16 maxv Exp $

.include <bsd.own.mk>

MAN=	nvmmctl.8

.if (${MACHINE_ARCH} == "x86_64")
USE_FORT?=yes	# setgid
PROG=nvmmctl
SRCS=nvmmctl.c

BINGRP=nvmm
BINMODE=2555

CPPFLAGS+=-D_KERNTYPES
LDADD+=-lutil -lnvmm
DPADD+=${LIBUTIL} ${LIBNVMM}
.endif

.include <bsd.prog.mk>
