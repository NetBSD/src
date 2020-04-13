#	$NetBSD: Makefile,v 1.7.46.1 2020/04/13 08:05:54 martin Exp $

.if ${MACHINE_ARCH} == "i386"
PROG=	ipwctl
.endif

MAN=	ipwctl.8

COPTS.ipwctl.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
