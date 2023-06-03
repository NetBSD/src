#	$NetBSD: Makefile,v 1.9 2023/06/03 09:09:21 lukem Exp $

.if ${MACHINE_ARCH} == "i386"
PROG=	ipwctl
.endif

MAN=	ipwctl.8

COPTS.ipwctl.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
