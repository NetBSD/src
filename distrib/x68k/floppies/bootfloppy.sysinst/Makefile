#	$NetBSD: Makefile,v 1.11 2002/08/18 14:43:16 isaki Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

FLOPPYBASE=	sysinst
FLOPPYSIZE=	2400
BOOTNAME!=	printf "USTAR.volsize.%o" ${FLOPPYSIZE}
FLOPPYFILES=	${BOOTNAME} netbsd
FLOPPYSUFFIX=	.fs

FLOPPY_RELEASEDIR=	installation/floppy

MDEC=		${DESTDIR}/usr/mdec
KERNOBJ!=	cd ${.CURDIR}/../instkernel && ${PRINTOBJDIR}
FLOPPY_NETBSD=	${KERNOBJ}/netbsd.INSTALL.gz
# XXXDISTRIB: use MI installboot
FLOPPYINSTBOOT=\
	"dd if=${MDEC}/fdboot_ustar of=@IMAGE@ bs=8k count=1 conv=sync,notrunc"

CLEANFILES+=	${BOOTNAME}

${BOOTNAME}: ${MDEC}/boot
	@echo "Copying ${.ALLSRC} to ${.TARGET}"
	@rm -f ${.TARGET}
	@cp ${.ALLSRC} ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
