#	$NetBSD: Makefile,v 1.10 2002/05/02 13:04:49 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

FLOPPYBASE=	cdhdtape
FLOPPYSIZE=	5760
FLOPPYMETAFILE!=printf "tapelabel USTAR.volsize.%o" ${FLOPPYSIZE}
FLOPPYFILES=	boot ${FLOPPYMETAFILE} netbsd
FLOPPYMAX=	1

MDEC=		${DESTDIR}/usr/mdec
KERNOBJ!=	cd ${.CURDIR}/../instkernel && ${PRINTOBJDIR}
FLOPPY_NETBSD=	${KERNOBJ}/netbsd.INSTALL.gz
FLOPPY_BOOT=	${MDEC}/ustarboot
FLOPPYINSTBOOT=\
	"${INSTALLBOOT} -m ${MACHINE} -t raw -v -b 17 @IMAGE@ ${FLOPPY_BOOT}"

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
