#	$NetBSD: Makefile,v 1.11 2001/05/08 02:15:39 sommerfeld Exp $

.include <bsd.own.mk>

SUBDIR= kern fs uvm
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
