#	$NetBSD: Makefile,v 1.4.2.1 2004/08/03 10:37:31 skrll Exp $

.include <bsd.own.mk>

.if ${USETOOLS} == "yes"
PROG=		fixcoff
.include <bsd.prog.mk>
.else
HOSTPROG=	fixcoff
.include <bsd.hostprog.mk>
.endif
