#	$NetBSD: Makefile,v 1.12 2020/09/08 16:39:36 jmcneill Exp $

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

.if ${MACHINE_ARCH} == "aarch64" || ${MACHINE_ARCH} == "aarch64eb"
SUBDIR+= ../../../stand/efiboot/bootaa64
.endif

.if ${MACHINE_ARCH} == "earmv7hf" || ${MACHINE_ARCH} == "earmv7hfeb"
SUBDIR+= ../../../stand/efiboot/bootarm
.endif

.include <bsd.subdir.mk>
