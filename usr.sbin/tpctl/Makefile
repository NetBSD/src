#	$NetBSD: Makefile,v 1.4.34.1 2014/05/22 11:43:10 yamt Exp $

.include <bsd.own.mk>

.if ${MACHINE_CPU} == "arm" || ${MACHINE} == "hpcmips" || ${MACHINE} == "hpcsh"
PROG=	tpctl
SRCS=	main.c fb.c tp.c data.c
.endif

MAN=	tpctl.8

.include <bsd.prog.mk>
