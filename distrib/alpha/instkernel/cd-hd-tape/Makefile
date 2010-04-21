#	$NetBSD: Makefile,v 1.19.40.1 2010/04/21 05:20:39 matt Exp $

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
