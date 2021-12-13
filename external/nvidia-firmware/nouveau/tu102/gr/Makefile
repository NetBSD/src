# $NetBSD: Makefile,v 1.1 2021/12/13 23:59:40 maya Exp $

NOMAN=	# defined

FILES+=		${.CURDIR}/../../dist/tu102/gr/fecs_bl.bin
FILES+=		${.CURDIR}/../../dist/tu102/gr/fecs_data.bin
FILES+=		${.CURDIR}/../../dist/tu102/gr/fecs_inst.bin
FILES+=		${.CURDIR}/../../dist/tu102/gr/fecs_sig.bin
FILES+=		${.CURDIR}/../../dist/tu102/gr/gpccs_bl.bin
FILES+=		${.CURDIR}/../../dist/tu102/gr/gpccs_data.bin
FILES+=		${.CURDIR}/../../dist/tu102/gr/gpccs_inst.bin
FILES+=		${.CURDIR}/../../dist/tu102/gr/gpccs_sig.bin
FILES+=		${.CURDIR}/../../dist/tu102/gr/sw_bundle_init.bin
FILES+=		${.CURDIR}/../../dist/tu102/gr/sw_ctx.bin
FILES+=		${.CURDIR}/../../dist/tu102/gr/sw_method_init.bin
FILES+=		${.CURDIR}/../../dist/tu102/gr/sw_nonctx.bin
FILES+=		${.CURDIR}/../../dist/tu102/gr/sw_veid_bundle_init.bin

FILESDIR+=	${FIRMWAREDIR}/nouveau/nvidia/tu102/gr

SYMLINKS+=	${FIRMWAREDIR}/nouveau/nvidia/tu102/gr/fecs_bl.bin \
		${FIRMWAREDIR}/nouveau/nvidia/tu104/gr/fecs_bl.bin
SYMLINKS+=	${FIRMWAREDIR}/nouveau/nvidia/tu102/gr/gpccs_bl.bin \
		${FIRMWAREDIR}/nouveau/nvidia/tu104/gr/gpccs_bl.bin

SYMLINKS+=	${FIRMWAREDIR}/nouveau/nvidia/tu102/gr/fecs_bl.bin \
		${FIRMWAREDIR}/nouveau/nvidia/tu106/gr/fecs_bl.bin
SYMLINKS+=	${FIRMWAREDIR}/nouveau/nvidia/tu102/gr/gpccs_bl.bin \
		${FIRMWAREDIR}/nouveau/nvidia/tu106/gr/gpccs_bl.bin

.include <bsd.links.mk>
.include <bsd.files.mk>
