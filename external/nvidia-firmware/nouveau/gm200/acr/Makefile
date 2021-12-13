# $NetBSD: Makefile,v 1.1 2021/12/13 23:59:36 maya Exp $

NOMAN=	# defined

.include <bsd.own.mk>

FILES+=		${.CURDIR}/../../dist/gm200/acr/bl.bin
FILES+=		${.CURDIR}/../../dist/gm200/acr/ucode_load.bin
FILES+=		${.CURDIR}/../../dist/gm200/acr/ucode_unload.bin

FILESDIR+=	${FIRMWAREDIR}/nouveau/nvidia/gm200/acr

SYMLINKS+=	${FIRMWAREDIR}/nouveau/nvidia/gm200/acr/bl.bin \
		${FIRMWAREDIR}/nouveau/nvidia/gm204/acr/bl.bin
SYMLINKS+=	${FIRMWAREDIR}/nouveau/nvidia/gm200/acr/ucode_load.bin \
		${FIRMWAREDIR}/nouveau/nvidia/gm204/acr/ucode_load.bin
SYMLINKS+=	${FIRMWAREDIR}/nouveau/nvidia/gm200/acr/ucode_unload.bin \
		${FIRMWAREDIR}/nouveau/nvidia/gm204/acr/ucode_unload.bin

SYMLINKS+=	${FIRMWAREDIR}/nouveau/nvidia/gm200/acr/bl.bin \
		${FIRMWAREDIR}/nouveau/nvidia/gm206/acr/bl.bin


.include <bsd.links.mk>
.include <bsd.files.mk>
