#	$NetBSD: Makefile,v 1.22 2009/11/02 15:44:12 plunky Exp $

.include <bsd.own.mk>

SUBDIR+= csu libc libm libposix libpthread librt libutil

.if (${MACHINE_CPU} != "alpha" && \
     ${MACHINE_CPU} != "mips" && \
     ${MACHINE_CPU} != "hppa")
#SUBDIR+= libssp
.endif

.if (${MKSKEY} != "no")
SUBDIR+= libskey
.endif

.if ${MKCRYPTO} != "no"
SUBDIR+= libdes
.endif

.include <bsd.subdir.mk>
