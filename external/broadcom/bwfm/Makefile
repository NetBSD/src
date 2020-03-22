# $NetBSD: Makefile,v 1.4 2020/03/22 20:21:53 thorpej Exp $

NOMAN=	# define

.include <bsd.own.mk>

# Only install the SDIO firmware binaries on platforms where it will
# be useful.
.if ${MACHINE} == "evbarm"
BWFM_WANT_SDIO_FIRMWARE=	yes
.endif

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
.endif

FILESDIR=	${FIRMWAREDIR}/if_bwfm

.include <bsd.files.mk>
