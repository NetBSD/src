#	$NetBSD: Makefile,v 1.8 2019/10/13 07:28:19 mrg Exp $

.if ${MACHINE_ARCH} == "i386"
PROG=	ipwctl
.endif

MAN=	ipwctl.8

COPTS.ipwctl.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
