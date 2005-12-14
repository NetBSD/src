#	$NetBSD: Makefile,v 1.12 2005/12/14 23:56:45 rpaulo Exp $

.include <bsd.own.mk>

SUBDIR= kern fs uvm net
.if exists(arch/${MACHINE}/Makefile)
SUBDIR+= arch/${MACHINE}
.endif
.if exists(arch/${MACHINE_ARCH}/Makefile)
SUBDIR+= arch/${MACHINE_ARCH}
.endif
.if ${MKCRYPTO} != "no"
SUBDIR+=	crypto
.endif

SUBDIR := ${SUBDIR:O:u}

.include <bsd.subdir.mk>
