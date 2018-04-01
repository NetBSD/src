#	$NetBSD: Makefile,v 1.6 2018/04/01 04:35:06 ryo Exp $

.include <bsd.own.mk>

.if ${MACHINE_CPU} == "aarch64" || \
    ${MACHINE_CPU} == "arm" || \
    ${MACHINE} == "hpcmips" || \
    ${MACHINE} == "hpcsh"
PROG=	tpctl
SRCS=	main.c fb.c tp.c data.c
.endif

MAN=	tpctl.8

.include <bsd.prog.mk>
