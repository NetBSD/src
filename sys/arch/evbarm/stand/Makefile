#	$NetBSD: Makefile,v 1.10 2019/03/30 13:54:09 jmcneill Exp $

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

.if ${MACHINE_CPU} == "aarch64"
SUBDIR+= ../../../stand/efiboot/bootaa64
.endif

.if ${MACHINE_ARCH} == "earmv7hf"
SUBDIR+= ../../../stand/efiboot/bootarm
.endif

.include <bsd.subdir.mk>
