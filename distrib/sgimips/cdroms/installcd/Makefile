#	$NetBSD: Makefile,v 1.1.2.2 2007/03/09 06:59:37 riz Exp $
CDBASE=		sgimipscd		# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDKERNELS=	netbsd-INSTALL32_IP2x.gz ip2x
CDKERNELS+=	netbsd-INSTALL32_IP3x.gz ip3x
CDINSTKERNEL=	../../instkernel

SGIVOLHDR.size= 4096

image_md_post:
	@echo "Prepending SGI volume header:"
	mv ${CDBASE}.iso ${CDBASE}.iso.tmp
	dd if=/dev/zero of=${CDBASE}.iso bs=512 count=${SGIVOLHDR.size}
	dd if=${CDBASE}.iso.tmp of=${CDBASE}.iso bs=512 seek=${SGIVOLHDR.size}
	${TOOLDIR}/bin/nbsgivol -f -i -h ${SGIVOLHDR.size} ${CDBASE}.iso
	${TOOLDIR}/bin/nbsgivol -f -w aoutboot ${DESTDIR}/usr/mdec/aoutboot ${CDBASE}.iso
	${TOOLDIR}/bin/nbsgivol -f -w ip2xboot ${DESTDIR}/usr/mdec/ip2xboot ${CDBASE}.iso
	${TOOLDIR}/bin/nbsgivol -f -w ip3xboot ${DESTDIR}/usr/mdec/ip3xboot ${CDBASE}.iso
	${RM} -f ${CDBASE}.iso.tmp

.include "${.CURDIR}/../../../common/Makefile.bootcd"

