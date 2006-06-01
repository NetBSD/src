#	$NetBSD: Makefile,v 1.3.16.1 2006/06/01 21:43:57 kardel Exp $

PROG=	ntptime
MAN=	ntptime.8
SRCS=	ntptime.c

.include <bsd.own.mk>

.if ${HAVE_GCC} == 4
COPTS+= -fno-strict-aliasing
.endif

NTPSRCDIR=util

.include <bsd.prog.mk>
