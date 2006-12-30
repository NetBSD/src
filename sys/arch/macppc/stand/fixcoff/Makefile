#	$NetBSD: Makefile,v 1.7.16.1 2006/12/30 20:46:30 yamt Exp $

NOMAN=		# defined
.include <bsd.own.mk>

.if ${USETOOLS} == "yes"
PROG=		fixcoff
.include <bsd.prog.mk>
.else
HOSTPROG=	fixcoff
.include <bsd.hostprog.mk>
.endif
