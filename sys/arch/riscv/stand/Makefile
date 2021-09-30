#	$NetBSD: Makefile,v 1.1 2021/09/30 20:02:55 jmcneill Exp $

.include <bsd.own.mk>

.if ${HAVE_UEFI:Uno} == "yes"
. if ${MACHINE_ARCH} == "riscv64"
SUBDIR+= ../../../stand/efiboot/bootriscv64
. endif
.endif

.include <bsd.subdir.mk>
