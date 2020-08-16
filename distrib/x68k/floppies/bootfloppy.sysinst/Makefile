#	$NetBSD: Makefile,v 1.14 2020/08/16 07:03:45 isaki Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

FLOPPYBASE=	sysinst
FLOPPYSIZE=	2400
BOOTNAME!=	printf "USTAR.volsize.%o" ${FLOPPYSIZE}
FLOPPYFILES=	${BOOTNAME} netbsd
FLOPPYSUFFIX=	.fs
FLOPPYPAD=	1

FLOPPY_RELEASEDIR=	installation/floppy

MDEC=		${DESTDIR}/usr/mdec
KERNOBJ!=	cd ${.CURDIR}/../instkernel && ${PRINTOBJDIR}
FLOPPY_NETBSD=	${KERNOBJ}/netbsd-INSTALL.gz
# XXXDISTRIB: use MI installboot
FLOPPYINSTBOOT=\
	"dd if=${MDEC}/xxboot_ustarfs of=@IMAGE@ bs=8k count=1 conv=sync,notrunc"

CLEANFILES+=	${BOOTNAME}

${BOOTNAME}: ${MDEC}/boot
	@echo "Copying ${.ALLSRC} to ${.TARGET}"
	@rm -f ${.TARGET}
	@cp ${.ALLSRC} ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
