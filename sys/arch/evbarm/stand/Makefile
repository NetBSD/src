#	$NetBSD: Makefile,v 1.15 2026/02/01 11:31:29 yurix Exp $

.include <bsd.endian.mk>

.if ${MACHINE_CPU} == "arm"
SUBDIR+= gzboot
SUBDIR+= boot2440
.endif

.if ${HAVE_UEFI:Uno} == "yes"
. if !empty(MACHINE_ARCH:Maarch64*)
SUBDIR+= ../../../stand/efiboot/bootaa64
. else
SUBDIR+= ../../../stand/efiboot/bootarm
. endif
.endif

.include <bsd.subdir.mk>
