#	$NetBSD: Makefile,v 1.3 2020/09/26 17:49:50 jmcneill Exp $

.include <bsd.own.mk>

MAN=	nvmmctl.8

.if ${HAVE_NVMM:Uno} == "yes"
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
