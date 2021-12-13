# $NetBSD: Makefile,v 1.1 2021/12/13 23:59:42 maya Exp $

NOMAN=	# defined

FILES+=		${.CURDIR}/../../dist/tu116/sec2/desc.bin
FILES+=		${.CURDIR}/../../dist/tu116/sec2/image.bin
FILES+=		${.CURDIR}/../../dist/tu116/sec2/sig.bin

FILESDIR+=	${FIRMWAREDIR}/nouveau/nvidia/tu116/sec2

SYMLINKS+=	${FIRMWAREDIR}/nouveau/nvidia/tu116/sec2/desc.bin \
		${FIRMWAREDIR}/nouveau/nvidia/tu117/sec2/desc.bin
SYMLINKS+=	${FIRMWAREDIR}/nouveau/nvidia/tu116/sec2/image.bin \
		${FIRMWAREDIR}/nouveau/nvidia/tu117/sec2/image.bin
SYMLINKS+=	${FIRMWAREDIR}/nouveau/nvidia/tu116/sec2/sig.bin \
		${FIRMWAREDIR}/nouveau/nvidia/tu117/sec2/sig.bin

.include <bsd.links.mk>
.include <bsd.files.mk>
