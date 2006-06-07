#	$NetBSD: Makefile,v 1.7 2006/06/07 20:59:28 kardel Exp $

PROG=	ntptime
MAN=	ntptime.8
SRCS=	ntptime.c

.include <bsd.own.mk>

.if ${HAVE_GCC} == 4
COPTS+= -fno-strict-aliasing
.endif

NTPSRCDIR=util

.include <bsd.prog.mk>
