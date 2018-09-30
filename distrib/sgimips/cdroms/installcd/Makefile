#	$NetBSD: Makefile,v 1.2.40.1 2018/09/30 01:45:05 pgoyette Exp $
CDBASE=		sgimipscd		# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDRELEASE_NODEBUG=	true
CDKERNELS=	netbsd-INSTALL32_IP2x.gz ip2x
CDKERNELS+=	netbsd-INSTALL32_IP3x.gz ip3x
CDINSTKERNEL=	../../instkernel

SGIVOLHDR.size= 4096

image_md_post:
	@echo "Prepending SGI volume header:"
	mv ${CDIMAGE} ${CDIMAGE}.tmp
	dd if=/dev/zero of=${CDIMAGE} bs=512 count=${SGIVOLHDR.size}
	dd if=${CDIMAGE}.tmp of=${CDIMAGE} bs=512 seek=${SGIVOLHDR.size}
	${TOOLDIR}/bin/nbsgivol -f -i -h ${SGIVOLHDR.size} ${CDIMAGE}
	${TOOLDIR}/bin/nbsgivol -f -w aoutboot ${DESTDIR}/usr/mdec/aoutboot ${CDIMAGE}
	${TOOLDIR}/bin/nbsgivol -f -w ip2xboot ${DESTDIR}/usr/mdec/ip2xboot ${CDIMAGE}
	${TOOLDIR}/bin/nbsgivol -f -w ip3xboot ${DESTDIR}/usr/mdec/ip3xboot ${CDIMAGE}
	${RM} -f ${CDIMAGE}.tmp

.include "${.CURDIR}/../../../common/Makefile.bootcd"

