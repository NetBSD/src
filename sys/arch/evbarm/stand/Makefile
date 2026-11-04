#	$NetBSD: Makefile,v 1.14.10.1 2026/11/04 23:20:30 perseant Exp $

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
