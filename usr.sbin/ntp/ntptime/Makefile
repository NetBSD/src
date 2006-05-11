#	$NetBSD: Makefile,v 1.6 2006/05/11 10:32:37 tron Exp $

PROG=	ntptime
MAN=	ntptime.8
SRCS=	ntptime.c

.include <bsd.own.mk>

.if ${HAVE_GCC} == 4
COPTS+= -fno-strict-aliasing
.endif

NTPSRCDIR=util

.include <bsd.prog.mk>
