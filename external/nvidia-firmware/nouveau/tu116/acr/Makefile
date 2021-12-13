# $NetBSD: Makefile,v 1.1 2021/12/13 23:59:41 maya Exp $

NOMAN=	# defined

FILES+=		${.CURDIR}/../../dist/tu116/acr/bl.bin
FILES+=		${.CURDIR}/../../dist/tu116/acr/ucode_ahesasc.bin
FILES+=		${.CURDIR}/../../dist/tu116/acr/ucode_asb.bin
FILES+=		${.CURDIR}/../../dist/tu116/acr/ucode_unload.bin
FILES+=		${.CURDIR}/../../dist/tu116/acr/unload_bl.bin

FILESDIR+=	${FIRMWAREDIR}/nouveau/nvidia/tu116/acr

SYMLINKS+=	${FIRMWAREDIR}/nouveau/nvidia/tu116/acr/bl.bin \
		${FIRMWAREDIR}/nouveau/nvidia/tu117/acr/bl.bin
SYMLINKS+=	${FIRMWAREDIR}/nouveau/nvidia/tu116/acr/ucode_ahesasc.bin \
		${FIRMWAREDIR}/nouveau/nvidia/tu117/acr/ucode_ahesasc.bin
SYMLINKS+=	${FIRMWAREDIR}/nouveau/nvidia/tu116/acr/ucode_asb.bin \
		${FIRMWAREDIR}/nouveau/nvidia/tu117/acr/ucode_asb.bin
SYMLINKS+=	${FIRMWAREDIR}/nouveau/nvidia/tu116/acr/unload_bl.bin \
		${FIRMWAREDIR}/nouveau/nvidia/tu117/acr/unload_bl.bin
SYMLINKS+=	${FIRMWAREDIR}/nouveau/nvidia/tu116/acr/ucode_unload.bin \
		${FIRMWAREDIR}/nouveau/nvidia/tu117/acr/ucode_unload.bin

.include <bsd.links.mk>
.include <bsd.files.mk>
