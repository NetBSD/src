#	$NetBSD: Makefile,v 1.4.2.3 2004/09/21 13:18:30 skrll Exp $

.include <bsd.own.mk>

.if ${USETOOLS} == "yes"
PROG=		fixcoff
.include <bsd.prog.mk>
.else
HOSTPROG=	fixcoff
.include <bsd.hostprog.mk>
.endif
