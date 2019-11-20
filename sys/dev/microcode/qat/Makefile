#	$NetBSD: Makefile,v 1.1 2019/11/20 09:37:45 hikaru Exp $

NOMAN=	# defined

.include <bsd.own.mk>

.if ${MKFIRMWARE} != "no"
FILES=	qat-license

FILES+=	mmp_firmware_c2xxx.bin mof_firmware_c2xxx.bin
FILES+=	qat_c3xxx.bin qat_c3xxx_mmp.bin
FILES+=	qat_c62x.bin qat_c62x_mmp.bin
FILES+=	qat_d15xx.bin qat_d15xx_mmp.bin
.endif

# XXX
includes:

clean:

cleandir:

.include <bsd.files.mk>
