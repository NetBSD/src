#	$NetBSD: Makefile,v 1.4.2.2 2004/09/18 14:37:09 skrll Exp $

.include <bsd.own.mk>

.if ${USETOOLS} == "yes"
PROG=		fixcoff
.include <bsd.prog.mk>
.else
HOSTPROG=	fixcoff
.include <bsd.hostprog.mk>
.endif
