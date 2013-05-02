#	$NetBSD: Makefile,v 1.5 2013/05/02 03:56:42 matt Exp $

.include <bsd.own.mk>

.if ${MACHINE_CPU} == "arm" || ${MACHINE} == "hpcmips" || ${MACHINE} == "hpcsh"
PROG=	tpctl
SRCS=	main.c fb.c tp.c data.c
.endif

MAN=	tpctl.8

.include <bsd.prog.mk>
