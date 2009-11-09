#	$NetBSD: Makefile,v 1.20 2009/11/09 04:13:23 mhitch Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

FLOPPYBASE=	cdhdtape
FLOPPYSIZE=	10000
FLOPPYMETAFILE!=printf "USTAR.volsize.%o" ${FLOPPYSIZE}
FLOPPYFILES=	boot ${FLOPPYMETAFILE} netbsd
FLOPPYMAX=	1
FLOPPYPAD=	1

FLOPPY_RELEASEDIR=	installation/diskimage

MDEC=		${DESTDIR}/usr/mdec
KERNOBJ!=	cd ${.CURDIR}/../instkernel && ${PRINTOBJDIR}
FLOPPY_NETBSD=	${KERNOBJ}/netbsd.gz
FLOPPY_BOOT=	${MDEC}/ustarboot
FLOPPYINSTBOOT=	"${TOOL_INSTALLBOOT} -v -m ${MACHINE} -t raw -b 17 @IMAGE@ ${FLOPPY_BOOT}"

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
