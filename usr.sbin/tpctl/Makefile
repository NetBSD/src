#	$NetBSD: Makefile,v 1.2 2006/05/22 07:36:52 mrg Exp $

.if ${MACHINE} == "hpcarm" || ${MACHINE} == "hpcmips" || ${MACHINE} == "hpcsh"
PROG=	tpctl
SRCS=	main.c fb.c tp.c data.c
.endif

MAN=	tpctl.8

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS.tp.c+=	-Wno-pointer-sign
.endif
