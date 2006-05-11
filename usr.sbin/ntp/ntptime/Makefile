#	$NetBSD: Makefile,v 1.5 2006/05/11 07:15:17 mrg Exp $

PROG=	ntptime
MAN=	ntptime.8
SRCS=	ntptime.c

.if ${HAVE_GCC} == 4
COPTS+= -fno-strict-aliasing
.endif

NTPSRCDIR=util

.include <bsd.prog.mk>
