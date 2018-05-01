#	$NetBSD: Makefile,v 1.10 2018/05/01 19:59:45 christos Exp $

.if ${USETOOLS} == "yes"
.include <bsd.init.mk>
PROG=		fixcoff
.include <bsd.prog.mk>
.else
.include <bsd.hostinit.mk>
HOSTPROG=	fixcoff
.include <bsd.hostprog.mk>
.endif
