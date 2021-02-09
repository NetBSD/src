# $NetBSD: Makefile,v 1.9 2021/02/09 11:57:20 yamt Exp $

NOMAN=	# define

.include <bsd.own.mk>

# Only install the SDIO firmware binaries on platforms where it will
# be useful.
.if ${MACHINE} == "evbarm"
BWFM_WANT_SDIO_FIRMWARE=	yes
.endif

FILESDIR=	${FIRMWAREDIR}/if_bwfm

FILES=	dist/LICENCE.broadcom_bcm43xx

FILES+=	dist/brcmfmac43143.bin
FILES+=	dist/brcmfmac43236b.bin
FILES+=	dist/brcmfmac43242a.bin
FILES+=	dist/brcmfmac43569.bin
FILES+=	dist/brcmfmac4373.bin

FILES+=	dist/brcmfmac4350-pcie.bin
FILES+=	dist/brcmfmac4350c2-pcie.bin
FILES+=	dist/brcmfmac4356-pcie.bin
FILES+=	dist/brcmfmac43570-pcie.bin
FILES+=	dist/brcmfmac4358-pcie.bin
FILES+=	dist/brcmfmac43602-pcie.ap.bin
FILES+=	dist/brcmfmac43602-pcie.bin
FILES+=	dist/brcmfmac4366b-pcie.bin
FILES+=	dist/brcmfmac4366c-pcie.bin
FILES+=	dist/brcmfmac4371-pcie.bin

.if defined(BWFM_WANT_SDIO_FIRMWARE)
FILES+=	dist/brcmfmac43143-sdio.bin
FILES+=	dist/brcmfmac43241b0-sdio.bin
FILES+=	dist/brcmfmac43241b4-sdio.bin
FILES+=	dist/brcmfmac43241b5-sdio.bin
FILES+=	dist/brcmfmac4329-sdio.bin
FILES+=	dist/brcmfmac4330-sdio.bin
FILES+=	dist/brcmfmac4334-sdio.bin
FILES+=	dist/brcmfmac43340-sdio.bin
FILES+=	dist/brcmfmac4335-sdio.bin
FILES+=	dist/brcmfmac43362-sdio.bin
FILES+=	dist/brcmfmac4339-sdio.bin
FILES+=	dist/brcmfmac43430-sdio.bin
FILES+=	dist/brcmfmac43430a0-sdio.bin
FILES+=	dist/brcmfmac43455-sdio.bin
FILES+=	dist/brcmfmac43456-sdio.bin
FILES+=	dist/brcmfmac43456-sdio.clm_blob
FILES+=	dist/brcmfmac4354-sdio.bin
FILES+=	dist/brcmfmac4356-sdio.bin
FILES+=	dist/brcmfmac4373-sdio.bin
.endif	# BWFM_WANT_SDIO_FIRMWARE

# Platform-specific configuration files
.if ${MACHINE} == "evbarm"
FILES+=	dist/brcmfmac43362-sdio.cubietech,cubietruck.txt
FILES+=	dist/brcmfmac43362-sdio.lemaker,bananapro.txt
FILES+=	dist/brcmfmac43430-sdio.raspberrypi,3-model-b.txt
FILES+=	dist/brcmfmac43455-sdio.raspberrypi,3-model-b-plus.txt
FILES+=	dist/brcmfmac43455-sdio.raspberrypi,4-model-b.txt
FILES+=	dist/brcmfmac43456-sdio.pine64,pinebook-pro.txt

# The raspberrypi,model-zero-w uses the same nvram config as the
# raspberrypi,3-model-b.
LINKS+=	${FILESDIR}/brcmfmac43430-sdio.raspberrypi,3-model-b.txt \
	${FILESDIR}/brcmfmac43430-sdio.raspberrypi,model-zero-w.txt

# Boards that share the common AP6212 nvram config are listed here.
FILES+=	dist/brcmfmac43430-sdio.AP6212.txt
LINKS+=	${FILESDIR}/brcmfmac43430-sdio.AP6212.txt \
	${FILESDIR}/brcmfmac43430-sdio.sinovoip,bpi-m2-zero.txt

# In lieu of a better option the xunlong,orangepi-lite2 has successfully
# used the the same nvram config as the raspberrypi,3-model-b-plus.
LINKS+=	${FILESDIR}/brcmfmac43455-sdio.raspberrypi,3-model-b-plus.txt \
	${FILESDIR}/brcmfmac43455-sdio.xunlong,orangepi-lite2.txt

# The model A+ has successully used the same nvram config as the B+.
# cf. https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/commit/?id=923bfa68a1c97929652f029da699495f4ce43f14
LINKS+=	${FILESDIR}/brcmfmac43455-sdio.raspberrypi,3-model-b-plus.txt \
    ${FILESDIR}/brcmfmac43455-sdio.raspberrypi,3-model-a-plus.txt
.endif

.include <bsd.links.mk>
.include <bsd.files.mk>
