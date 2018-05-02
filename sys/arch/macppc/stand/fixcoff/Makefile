#	$NetBSD: Makefile,v 1.9.144.1 2018/05/02 07:20:05 pgoyette Exp $

.if ${USETOOLS} == "yes"
.include <bsd.init.mk>
PROG=		fixcoff
.include <bsd.prog.mk>
.else
.include <bsd.hostinit.mk>
HOSTPROG=	fixcoff
.include <bsd.hostprog.mk>
.endif
