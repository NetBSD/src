#	$NetBSD: Makefile,v 1.9 2006/09/16 00:57:49 uwe Exp $

NOMAN=		# defined
.include <bsd.own.mk>

.if ${USETOOLS} == "yes"
PROG=		fixcoff
.include <bsd.prog.mk>
.else
HOSTPROG=	fixcoff
.include <bsd.hostprog.mk>
.endif
