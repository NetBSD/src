#	$NetBSD: Makefile,v 1.10 2000/11/01 15:34:28 itojun Exp $

.include <bsd.own.mk>

SUBDIR= kern fs uvm
.if exists(arch/${MACHINE})
SUBDIR+= arch/${MACHINE}
.endif
.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif
.if ${MKCRYPTO} != "no"
SUBDIR+=	crypto
.endif

.include <bsd.subdir.mk>
