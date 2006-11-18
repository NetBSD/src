#	$NetBSD: Makefile,v 1.8.20.1 2006/11/18 21:29:24 ad Exp $

NOMAN=		# defined
.include <bsd.own.mk>

.if ${USETOOLS} == "yes"
PROG=		fixcoff
.include <bsd.prog.mk>
.else
HOSTPROG=	fixcoff
.include <bsd.hostprog.mk>
.endif
