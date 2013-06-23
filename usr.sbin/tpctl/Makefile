#	$NetBSD: Makefile,v 1.4.40.1 2013/06/23 06:29:06 tls Exp $

.include <bsd.own.mk>

.if ${MACHINE_CPU} == "arm" || ${MACHINE} == "hpcmips" || ${MACHINE} == "hpcsh"
PROG=	tpctl
SRCS=	main.c fb.c tp.c data.c
.endif

MAN=	tpctl.8

.include <bsd.prog.mk>
