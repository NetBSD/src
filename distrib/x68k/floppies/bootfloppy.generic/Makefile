#	$NetBSD: Makefile,v 1.4 2002/05/13 01:04:19 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

FLOPPYBASE=	kern
FLOPPYSIZE=	2400
FLOPPYMETAFILE!=printf "USTAR.volsize.%o" ${FLOPPYSIZE}
FLOPPYFILES=	boot ${FLOPPYMETAFILE} netbsd
FLOPPYSUFFIX=	.fs

FLOPPY_RELEASEDIR=	installation/floppy

MDEC=		${DESTDIR}/usr/mdec
KERNOBJ!=	cd ${.CURDIR}/../instkernel && ${PRINTOBJDIR}
FLOPPY_NETBSD=	${KERNOBJDIR}/GENERIC/netbsd
FLOPPY_BOOT=	${MDEC}/boot
# XXXDISTRIB: use MI installboot
FLOPPYINSTBOOT=\
	"dd if=${MDEC}/fdboot_ustar of=@IMAGE@ bs=8k count=1 conv=sync,notrunc"

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
