#	$NetBSD: Makefile,v 1.14 2021/09/16 20:17:47 andvar Exp $

.include <bsd.endian.mk>

.if ${MACHINE_CPU} == "arm"
SUBDIR+= gzboot
SUBDIR+= boot2440
.if ${TARGET_ENDIANNESS} == "1234"
# According to the i.MX23 Reference Manual section 3.1, Page 3-3:
# "The i.MX23 always operates in little-endian mode."
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
