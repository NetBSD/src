#	$NetBSD: Makefile,v 1.5 2002/05/19 08:12:05 isaki Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

FLOPPYBASE=	kern
FLOPPYSIZE=	2400
BOOTNAME!=	printf "USTAR.volsize.%o" ${FLOPPYSIZE}
FLOPPYFILES=	${BOOTNAME} netbsd
FLOPPYSUFFIX=	.fs

FLOPPY_RELEASEDIR=	installation/floppy

MDEC=		${DESTDIR}/usr/mdec
KERNOBJ!=	cd ${.CURDIR}/../instkernel && ${PRINTOBJDIR}
FLOPPY_NETBSD=	${KERNOBJDIR}/GENERIC/netbsd
# XXXDISTRIB: use MI installboot
FLOPPYINSTBOOT=\
	"dd if=${MDEC}/fdboot_ustar of=@IMAGE@ bs=8k count=1 conv=sync,notrunc"

${BOOTNAME}: ${MDEC}/boot
	@echo "Copying ${.ALLSRC} to ${.TARGET}"
	@rm -f ${.TARGET}
	@cp ${.ALLSRC} ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
