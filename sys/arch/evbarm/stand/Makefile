#	$NetBSD: Makefile,v 1.13 2020/09/12 15:25:41 jmcneill Exp $

.include <bsd.endian.mk>

.if ${MACHINE_CPU} == "arm"
SUBDIR+= gzboot
SUBDIR+= boot2440
.if ${TARGET_ENDIANNESS} == "1234"
# According to the i.MX23 Reference Manual section 3.1, Page 3-3:
# "The i.MX23 always operates in litle-endian mode."
SUBDIR+= bootimx23
.endif
.endif

.if ${HAVE_UEFI:Uno} == "yes"
. if !empty(MACHINE_ARCH:Maarch64*)
SUBDIR+= ../../../stand/efiboot/bootaa64
. else
SUBDIR+= ../../../stand/efiboot/bootarm
. endif
.endif

.include <bsd.subdir.mk>
